#include "inSituModule.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <vistle/insitu/core/slowMpi.h>
#include <vistle/insitu/message/InSituMessage.h>
#include <vistle/insitu/util/print.h>
#include <vistle/util/hostname.h>
#include <vistle/util/sleep.h>

using namespace vistle;
using namespace vistle::insitu;

#define CERR std::cerr << "InSituModule[" << rank() << "/" << size() << "] "
#define DEBUG_CERR vistle::DoNotPrintInstance

InSituModule::InSituModule(const std::string &name, const int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
, m_simulationCommandsComm(comm, boost::mpi::comm_duplicate)
, m_vistleObjectsComm(comm, boost::mpi::comm_duplicate)
{}

InSituModule::~InSituModule()
{
    if (m_messageHandler)
        m_messageHandler->send(message::ConnectionClosed(true));
    terminateCommunicationThreads();
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

void InSituModule::startCommunicationThreads()
{
    m_terminateCommunication = false;
    m_simulationCommandsThread = std::make_unique<std::thread>(std::bind(&InSituModule::communicateWithSim, this));
    m_vistleObjectsThread = std::make_unique<std::thread>(std::bind(&InSituModule::recvVistleObjects, this));
}

void InSituModule::terminateCommunicationThreads()
{
    m_messageHandler = nullptr;
    m_terminateCommunication = true;
    if (m_simulationCommandsThread && m_simulationCommandsThread->joinable()) {
        m_simulationCommandsThread->join();
    }
    if (m_vistleObjectsThread && m_vistleObjectsThread->joinable()) {
        m_vistleObjectsThread->join();
    }
}

void InSituModule::communicateWithSim()
{
    while (true) {
        bool terminateCommunication = m_terminateCommunication;
        boost::mpi::broadcast(m_simulationCommandsComm, terminateCommunication, 0);
        if (terminateCommunication)
            break;
        bool workDone = false;
        while (recvAndhandleMessage()) {
            workDone = true;
        }
        vistle::adaptive_wait(workDone, &m_simulationCommandsComm);
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
        m_vistleObjectsMessageQueue.reset(vistle::message::MessageQueue::create(msqName));
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

void InSituModule::initializeCommunication()
{
    if (m_messageHandler) {
        CERR << "can not connect to a new simulation before the old simulation is disconnected." << std::endl;
        return;
    }
    m_messageHandler = connectToSim();
    if (m_messageHandler) {
        initRecvFromSimQueue();
        startCommunicationThreads();
        comm().barrier(); //wait for all ranks to create msqs before sending the connection info
        m_messageHandler->send(message::ShmInfo{gatherModuleInfo()});
        sendIntOptions();
    }
}

const insitu::message::MessageHandler *InSituModule::getMessageHandler() const
{
    return m_messageHandler.get();
}

bool InSituModule::changeParameter(const Parameter *param)
{
    if (!param) {
        return true;
    }
    CERR << "changeParameter " << param->getName() << std::endl;
    if (m_filePath && param == m_filePath) {
        initializeCommunication();
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
        }
    }
    return true;
}

bool isPackageComplete(const vistle::message::Buffer &buf)
{
    return buf.type() == vistle::message::INSITU && buf.as<vistle::insitu::message::InSituMessage>().ismType() ==
                                                        vistle::insitu::message::InSituMessageType::PackageComplete;
}

bool InSituModule::prepare()
{
    std::lock_guard<std::mutex> g{m_vistleObjectsMutex};
    if (m_cachedVistleObjects.empty())
        return true;
    auto &objSet = m_cachedVistleObjects.front();
    for (auto &obj: objSet) {
        if (obj.type() == vistle::message::ADDOBJECT) {
            auto &addObj = obj.as<vistle::message::AddObject>();
            updateMeta(obj);
            passThroughObject(addObj.getSenderPort(), addObj.takeObject());
            CERR << "passed through object: timestep = " << addObj.meta().timeStep()
                 << " iteration = " << addObj.meta().iteration() << " generation = " << addObj.meta().generation()
                 << std::endl;
        }
    }
    m_cachedVistleObjects.pop();
    return true;
}

void InSituModule::updateMeta(const vistle::message::Buffer &obj)
{
    if (obj.type() == vistle::message::ADDOBJECT) {
        auto &objMsg = obj.as<vistle::message::AddObject>();
        m_iteration = objMsg.meta().iteration();
        m_generation = objMsg.meta().generation();
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
        DEBUG_CERR << "received message of type " << static_cast<int>(msg.type()) << std::endl;
    }
    return handleInsituMessage(msg);
}

void InSituModule::recvVistleObjects()
{
    while (!m_terminateCommunication) {
        if (cacheVistleObjects()) {
            execute();
            vistle::adaptive_wait(true, &m_vistleObjectsMutex);
        } else {
            vistle::adaptive_wait(false, &m_vistleObjectsMutex);
        }
    }
}


bool InSituModule::cacheVistleObjects()
{
    if (!m_vistleObjectsMessageQueue)
        return false;
    std::vector<vistle::message::Buffer> vistleObjects;
    while (!m_terminateCommunication) {
        vistle::message::Buffer buf;
        bool workDone = false;
        if (m_vistleObjectsMessageQueue->tryReceive(buf)) {
            workDone = true;
            if (isPackageComplete(buf)) {
                vistle::insitu::barrier(m_vistleObjectsComm, m_terminateCommunication);
                std::lock_guard<std::mutex> g{m_vistleObjectsMutex};
                m_cachedVistleObjects.emplace(std::move(vistleObjects));
                return true;
            }
            vistleObjects.emplace_back(std::move(buf));
        }
        vistle::adaptive_wait(workDone, &m_vistleObjectsMutex);
    }
    return false;
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
            const auto &name = (*i)->getName();
            if (std::find(commands.begin(), commands.end(), name) == commands.end()) {
                removeParameter(name);
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
        return false;
#endif
    } break;
    default:
        break;
    }
    return true;
}
