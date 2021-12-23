#include "inSituModule.h"
#include <vistle/util/sleep.h>
#include <vistle/util/hostname.h>
#include <vistle/insitu/util/print.h>
#include <vistle/insitu/message/InSituMessage.h>
#include <iostream>

#include <thread>
#include <chrono>

using namespace vistle;
using namespace vistle::insitu;

#define CERR std::cerr << "InSituModule[" << rank() << "/" << size() << "] "
#define DEBUG_CERR vistle::DoNotPrintInstance

InSituModule::InSituModule(const std::string &name, const int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{}

InSituModule::~InSituModule()
{
    terminateCommunicationThread();
}

bool InSituModule::isConnectedToSim() const
{
    return m_messageHandler != nullptr;
}


//private

void InSituModule::sendIntOptions()
{
    for (const auto &option: m_intOptions) {
        m_messageHandler->send(message::IntOption{{option->getName(), option->getValue()}});
    }
}

void InSituModule::disconnectSim()
{
    m_messageHandler.reset();
    m_terminateCommunication = true;
}

void InSituModule::startCommunicationThread()
{
    m_terminateCommunication = false;
    m_communicationThread = std::make_unique<std::thread>(std::bind(&InSituModule::communicateWithSim, this));
}

void InSituModule::terminateCommunicationThread()
{
    if (m_communicationThread && m_communicationThread->joinable()) {
        m_terminateCommunication = true;
        m_communicationThread->join();
    }
}

void InSituModule::communicateWithSim()
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
    if (m_messageHandler) {
        m_messageHandler->send(message::ConnectionClosed{true});
    }
}

void InSituModule::initRecvFromSimQueue()
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

vistle::insitu::message::ModuleInfo::ShmInfo InSituModule::gatherModuleInfo() const
{
    message::ModuleInfo::ShmInfo shmInfo;
    shmInfo.hostname = vistle::hostname();
    shmInfo.id = id();
    shmInfo.mpiSize = size();
    shmInfo.name = name();
    shmInfo.numCons = m_instanceNum;
    shmInfo.shmName = vistle::Shm::the().instanceName();
    CERR << "vistle::Shm::the().instanceName() = " << vistle::Shm::the().instanceName()
         << " vistle::Shm::the().name() = " << vistle::Shm::the().name() << std::endl;
    return shmInfo;
}

bool InSituModule::changeParameter(const Parameter *param)
{
    if (!param) {
        // if (m_connectedToSim) {
        //     sendIntOptions();
        // }
        return true;
    }
    if (param == m_filePath) {
        if (m_messageHandler) {
            CERR << "can not connect to a new simulation before the old simulation is disconnected." << std::endl;
            return true;
        }
        if (m_messageHandler = connectToSim()) {
            initRecvFromSimQueue();
            startCommunicationThread();

            m_messageHandler->send(message::ShmInfo{gatherModuleInfo()});
            sendIntOptions();
        }
        return true;
    }
    if (!m_messageHandler)
        return true;
    if (std::find(m_commandParameter.begin(), m_commandParameter.end(), param) != m_commandParameter.end()) {
        m_messageHandler->send(message::ExecuteCommand({param->getName(), ""}));
    } else if (auto intParam = dynamic_cast<const vistle::IntParameter *>(param)) {
        auto it = std::find(m_intOptions.begin(), m_intOptions.end(), intParam);
        if (it != m_intOptions.end()) {
            m_messageHandler->send(message::IntOption{{intParam->getName(), intParam->getValue()}});
            m_ownExecutionCounter++;
        }
    }
    return true;
}

bool InSituModule::prepare()
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

void InSituModule::updateMeta(const vistle::message::Buffer &obj)
{
    if (obj.type() == vistle::message::ADDOBJECT) {
        auto &objMsg = obj.as<vistle::message::AddObject>();
        //m_iteration = objMsg.meta().iteration();
        m_executionCount = objMsg.meta().executionCounter();
        std::cerr << "adding object for iteration " << m_iteration << " and executionCounter " << m_executionCount
                  << " and timestep " << objMsg.meta().timeStep() << std::endl;
    }
}

void InSituModule::connectionAdded(const Port *from, const Port *to)
{
    if (m_messageHandler && from->connections().size() < 2) // assumes the port got added before this call
    {
        m_messageHandler->send(message::ConnectPort{from->getName()});
    }
}

void InSituModule::connectionRemoved(const Port *from, const Port *to)
{
    if (m_messageHandler && from->connections().size() == 1) // assumes the port gets removed after this call
        m_messageHandler->send(message::DisconnectPort{from->getName()});
}

bool InSituModule::recvAndhandleMessage()
{
    auto msg = m_messageHandler->tryRecv();
    if (msg.type() != message::InSituMessageType::Invalid) {
        std::cerr << "received message of type " << static_cast<int>(msg.type()) << std::endl;
    }
    return handleInsituMessage(msg);
}

bool InSituModule::recvVistleObjects()
{
    std::lock_guard<std::mutex> g{m_communicationMutex};
    if (chacheVistleObjects()) {
        execute();
    }
    return !m_vistleObjects.empty();
}

bool InSituModule::chacheVistleObjects()
{
    bool retval = false;
    auto start = std::chrono::system_clock::now();
    auto duration = std::chrono::microseconds(100);
    while (m_receiveFromSimMessageQueue && (std::chrono::system_clock::now() < start + duration)) {
        vistle::message::Buffer buf;
        if (m_receiveFromSimMessageQueue->tryReceive(buf)) {
            if (buf.type() != vistle::message::INSITU) {
                m_vistleObjects.push_back(buf);
                retval = true;
            }
        }
    }
    return retval;
}

bool InSituModule::handleInsituMessage(message::Message &msg)
{
    using namespace vistle::insitu::message;
    DEBUG_CERR << "handleInsituMessage " << (int)msg.type() << std::endl;
    switch (msg.type()) {
    case InSituMessageType::Invalid:
        return false;
    case InSituMessageType::SetPorts: // list of ports, last entry is the type description (e.g mesh or variable)
    {
        auto em = msg.unpackOrCast<SetPorts>();
        for (auto i = m_outputPorts.begin(); i != m_outputPorts.end(); ++i) { // destoy unnecessary ports
            if (std::find_if(em.value.begin(), em.value.end(), [i](const std::vector<std::string> &ports) {
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
    case InSituMessageType::SetCommands:
    case InSituMessageType::SetCustomCommands: {
        auto commands = msg.type() == InSituMessageType::SetCommands ? msg.unpackOrCast<SetCommands>().value
                                                                     : msg.unpackOrCast<SetCustomCommands>().value;
        auto &params = msg.type() == InSituMessageType::SetCommands ? m_commandParameter : m_customCommandParameter;

        for (auto i = params.begin(); i != params.end(); ++i) {
            if (std::find(commands.begin(), commands.end(), (*i)->getName()) == commands.end()) {
                removeParameter(*i);
                i = params.erase(i);
            }
        }
        for (auto cmd: commands) {
            auto lb =
                std::find_if(params.begin(), params.end(), [cmd](const auto &val) { return val->getName() == cmd; });
            if (lb == params.end()) {
                if (msg.type() == InSituMessageType::SetCommands)
                    params.insert(addIntParameter(cmd, "trigger command on change", false,
                                                  vistle::Parameter::Presentation::Boolean));
                else
                    params.insert(addStringParameter(cmd, "trigger command on change", ""));
            }
        }
    } break;
    case InSituMessageType::GoOn: {
        m_messageHandler->send(GoOn{});
    } break;
    case InSituMessageType::ConnectionClosed: {
        auto state = msg.unpackOrCast<ConnectionClosed>();
        if (state.value) {
            CERR << "the simulation disconnected properly" << std::endl;
        } else {
            CERR << " tcp connection closed...disconnecting." << std::endl;
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
