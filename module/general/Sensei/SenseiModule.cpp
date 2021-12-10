#include "SenseiModule.h"

#include <vistle/core/rectilineargrid.h>
#include <vistle/insitu/util/print.h>
#include <vistle/util/directory.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/hostname.h>
#include <vistle/util/sleep.h>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/mpi.hpp>

using namespace std;
using namespace vistle;
using namespace vistle::insitu::sensei;
using namespace vistle::insitu::message;

#define CERR cerr << "SenseiModule[" << rank() << "/" << size() << "] "
#define DEBUG_CERR vistle::DoNotPrintInstance

SenseiModule::SenseiModule(const string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    m_filePath = addStringParameter("path", "path to the connection file written by the simulation",
                                    directory::configHome() + "/sensei.vistle", vistle::Parameter::ExistingFilename);
    setParameterFilters(m_filePath, "simulation Files (*.vistle)");

    m_deleteShm =
        addIntParameter("deleteShm", "clean the shared memory message queues used to communicate with the simulation",
                        false, vistle::Parameter::Presentation::Boolean);
    m_intOptions[addIntParameter("frequency", "the pipeline is processed for every nth simulation cycle", 1)] =
        sensei::IntOptions::NthTimestep;
    m_intOptions[addIntParameter("keep timesteps", "if true timesteps are cached and processed as time series", true,
                                 vistle::Parameter::Boolean)] = sensei::IntOptions::KeepTimesteps;

    connectToSim();
}

SenseiModule::~SenseiModule()
{
    terminateCommunicationThread();
}

void SenseiModule::connectToSim()
{
    if (m_connectedToSim) {
        CERR << "can not connect to a new simulation before the old simulation is disconnected." << std::endl;
        return;
    }

    CERR << "trying to connect to sim with file " << m_filePath->getValue() << endl;
    std::ifstream infile(m_filePath->getValue());
    if (infile.fail()) {
        CERR << "failed to open file " << m_filePath->getValue() << endl;
        return;
    }
    std::string key, rankStr;
    while (rankStr != std::to_string(rank())) {
        infile >> rankStr;
        infile >> key;
        if (infile.eof()) {
            CERR << "missing connection key for rank " << rank() << endl;
            break;
        }
    }

    infile.close();
    initRecvFromSimQueue();
    startCommunicationThread();
    CERR << " key = " << key << endl;
    m_messageHandler.initialize(key, m_rank);
    m_messageHandler.send(ShmInfo{gatherModuleInfo()});
    m_connectedToSim = true;
    sendIntOptions();
}

void SenseiModule::sendIntOptions()
{
    for (const auto &option: m_intOptions) {
        m_messageHandler.send(SenseiIntOption{{option.second, option.first->getValue()}});
        CERR << "intOption: " << option.first->getName() << " " << option.first->getValue() << std::endl;
    }
}

void SenseiModule::disconnectSim()
{
    m_connectedToSim = false;
    m_messageHandler.reset();
    m_terminateCommunication = true;
}

void SenseiModule::startCommunicationThread()
{
    m_terminateCommunication = false;
    m_communicationThread = std::make_unique<std::thread>(std::bind(&SenseiModule::communicateWithSim, this));
}

void SenseiModule::terminateCommunicationThread()
{
    if (m_communicationThread && m_communicationThread->joinable()) {
        m_terminateCommunication = true;
        m_communicationThread->join();
    }
}

void SenseiModule::communicateWithSim()
{
    while (!m_terminateCommunication) {
        bool workDone = false;
        while (recvAndhandleMessage()) {
            workDone = true;
        }
        if (recvVistleObjects()) {
            workDone = true;
        }
        vistle::adaptive_wait(workDone);
    }
    if (m_connectedToSim) {
        m_messageHandler.send(ConnectionClosed{true});
    }
}

void SenseiModule::initRecvFromSimQueue()
{
    std::string msqName = vistle::message::MessageQueue::createName(
        ("recvFromSim" + std::to_string(++m_instanceNum)).c_str(), id(), rank());
    std::cerr << "created msqName " << msqName << std::endl;
    try {
        m_receiveFromSimMessageQueue.reset(vistle::message::MessageQueue::create(msqName));
        CERR << "receiveFromSimMessageQueue name = " << msqName << std::endl;
    } catch (boost::interprocess::interprocess_exception &ex) {
        throw vistle::exception(std::string("opening recv from sim message queue with name ") + msqName + ": " +
                                ex.what());
    }
}

vistle::insitu::message::ModuleInfo::ShmInfo SenseiModule::gatherModuleInfo() const
{
    message::ModuleInfo::ShmInfo shmInfo;
    shmInfo.hostname = vistle::hostname();
    shmInfo.id = id();
    shmInfo.mpiSize = size();
    shmInfo.name = name();
    shmInfo.numCons = m_instanceNum;
    shmInfo.shmName = vistle::Shm::the().instanceName();
    CERR << "vistle::Shm::the().instanceName() = " << vistle::Shm::the().instanceName()
         << " vistle::Shm::the().name() = " << vistle::Shm::the().name() << endl;
    return shmInfo;
}

bool SenseiModule::changeParameter(const Parameter *param)
{
    if (!param) {
        CERR << "examinating no param" << std::endl;
        if (m_connectedToSim) {
            sendIntOptions();
        }

        return true;
    }
    CERR << "examinating " << param->getName() << std::endl;
    if (param == m_deleteShm) {
        m_messageHandler.removeShm();
    } else if (param == m_filePath) {
        connectToSim();
        return true;
    }
    std::lock_guard<std::mutex> g{m_communicationMutex};
    if (std::find(m_commandParameter.begin(), m_commandParameter.end(), param) != m_commandParameter.end()) {
        m_messageHandler.send(ExecuteCommand({param->getName(), ""}));
    } else {
        auto option = dynamic_cast<const vistle::IntParameter *>(param);
        auto it = m_intOptions.find(option);
        if (it != m_intOptions.end()) {
            m_messageHandler.send(SenseiIntOption{{it->second, option->getValue()}});
            m_ownExecutionCounter++;
        }
    }
    return true;
}
bool SenseiModule::prepare()
{
    std::lock_guard<std::mutex> g{m_communicationMutex};
    m_executionCount = m_ownExecutionCounter;
    for (const auto &obj: m_vistleObjects) {
        updateMeta(obj);
        sendMessage(obj);
    }
    m_vistleObjects.clear();
    return true;
}

void SenseiModule::updateMeta(const vistle::message::Buffer &obj)
{
    if (obj.type() == vistle::message::ADDOBJECT) {
        auto &objMsg = obj.as<vistle::message::AddObject>();
        m_iteration = objMsg.meta().iteration();
        m_executionCount = objMsg.meta().executionCounter();
    }
}

void SenseiModule::connectionAdded(const Port *from, const Port *to)
{
    m_messageHandler.send(ConnectPort{from->getName()});
}

void SenseiModule::connectionRemoved(const Port *from, const Port *to)
{
    m_messageHandler.send(DisconnectPort{from->getName()});
}

bool SenseiModule::recvAndhandleMessage()
{
    auto msg = m_messageHandler.tryRecv();
    if (msg.type() != message::InSituMessageType::Invalid) {
        std::cerr << "received message of type " << static_cast<int>(msg.type()) << std::endl;
    }
    return handleMessage(msg);
}

bool SenseiModule::recvVistleObjects()
{
    std::lock_guard<std::mutex> g{m_communicationMutex};
    if (chacheVistleObjects()) {
        CERR << "requesting execution" << std::endl;
        execute();
    }
    return !m_vistleObjects.empty();
}

bool SenseiModule::chacheVistleObjects()
{
    bool retval = false;
    vistle::message::Buffer buf;
    while (m_receiveFromSimMessageQueue && m_receiveFromSimMessageQueue->tryReceive(buf)) {
        if (buf.type() != vistle::message::INSITU) {
            m_vistleObjects.push_back(buf);
            retval = true;
        }
    }
    return retval;
}

bool SenseiModule::handleMessage(Message &msg)
{
    DEBUG_CERR << "handleMessage " << (int)msg.type() << endl;
    switch (msg.type()) {
    case InSituMessageType::Invalid:
        return false;
    case InSituMessageType::SetPorts: // list of ports, last entry is the type description (e.g mesh or variable)
    {
        auto em = msg.unpackOrCast<SetPorts>();
        for (auto i = m_outputPorts.begin(); i != m_outputPorts.end(); ++i) { // destoy unnecessary ports
            if (std::find_if(em.value.begin(), em.value.end(), [i](const std::vector<string> &ports) {
                    return std::find(ports.begin(), ports.end(), i->first) != ports.end();
                }) == em.value.end()) {
                destroyPort(i->second);
                i = m_outputPorts.erase(i);
            }
        }
        for (auto portList: em.value) {
            for (size_t i = 0; i < portList.size() - 1; i++) {
                auto lb = m_outputPorts.lower_bound(portList[i]);
                if (!(lb != m_outputPorts.end() && !(m_outputPorts.key_comp()(portList[i], lb->first)))) {
                    m_outputPorts.insert(
                        lb, std::make_pair(portList[i], createOutputPort(portList[i], portList[portList.size() - 1])));
                }
            }
        }

    } break;
    case InSituMessageType::SetCommands: {
        auto em = msg.unpackOrCast<SetCommands>();
        for (auto i = m_commandParameter.begin(); i != m_commandParameter.end(); ++i) {
            if (std::find(em.value.begin(), em.value.end(), (*i)->getName()) == em.value.end()) {
                removeParameter(*i);
                i = m_commandParameter.erase(i);
            }
        }
        for (auto portName: em.value) {
            auto lb = std::find_if(m_commandParameter.begin(), m_commandParameter.end(),
                                   [portName](const auto &val) { return val->getName() == portName; });
            if (lb == m_commandParameter.end()) {
                m_commandParameter
                    .insert(addIntParameter(portName, "trigger command on change", false,
                                            vistle::Parameter::Presentation::Boolean))
                    .first;
            }
        }
    } break;
    case InSituMessageType::GoOn: {
        m_messageHandler.send(GoOn{});
    } break;
    case InSituMessageType::ConnectionClosed: {
        auto state = msg.unpackOrCast<ConnectionClosed>();
        if (state.value) {
            CERR << "the simulation disconnected properly" << endl;
        } else {
            CERR << " tcp connection closed...disconnecting." << endl;
        }
#ifdef MODULE_THREAD
        sendMessage(vistle::message::Quit());
#else
        disconnectSim();
#endif
    } break;
    default:
        break;
    }
    return true;
}

MODULE_MAIN_THREAD(SenseiModule, boost::mpi::threading::multiple)
