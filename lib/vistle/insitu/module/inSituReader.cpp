#include "inSituReader.h"
#include <vistle/core/message.h>
#include <vistle/util/hostname.h>
#include <vistle/util/sleep.h>

using namespace vistle;
using namespace vistle::insitu;
using std::endl;
#define CERR std::cerr << "inSituReader[" << rank() << "/" << size() << "] "

size_t InSituReader::m_numInstances = 0;

InSituReader::InSituReader(const std::string &name, const int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    setReducePolicy(vistle::message::ReducePolicy::OverAll);
}

bool InSituReader::isExecuting()
{
    return m_isExecuting;
}

bool InSituReader::handleExecute(const vistle::message::Execute *exec)
{
    using namespace vistle::message;

    if (m_executionCount < exec->getExecutionCount()) {
        m_executionCount = exec->getExecutionCount();
        m_iteration = -1;
    }

    bool ret = true;

#ifdef DETAILED_PROGRESS
    Busy busy;
    busy.setReferrer(exec->uuid());
    busy.setDestId(Id::LocalManager);
    sendMessage(busy);
#endif
    if (!m_isExecuting && (exec->what() == Execute::ComputeExecute || exec->what() == Execute::Prepare)) {
        applyDelayedChanges();
        ret &= prepareWrapper(exec);
        if (ret) {
            m_exec = exec;
            m_isExecuting = true;
        }
        return ret;
    }

#ifdef DETAILED_PROGRESS
    message::Idle idle;
    idle.setReferrer(exec->uuid());
    idle.setDestId(Id::LocalManager);
    sendMessage(idle);
#endif
    return ret;
}

bool InSituReader::operate()
{
    return false;
}

bool InSituReader::dispatch(bool block, bool *messageReceived, unsigned int minPrio)
{
    bool passMsg = false;
    bool msgRecv = false;
    vistle::message::Buffer buf;
    if (m_receiveFromSimMessageQueue && m_receiveFromSimMessageQueue->tryReceive(buf)) {
        if (buf.type() != vistle::message::INSITU) {
            sendMessage(buf);
        }
        passMsg = true;
    }
    bool retval = Module::dispatch(false, &msgRecv, minPrio);
    vistle::adaptive_wait(operate() || msgRecv || passMsg);
    if (messageReceived) {
        *messageReceived = msgRecv;
    }
    return retval;
}

bool InSituReader::prepare()
{
    try {
        m_shmIDs.set(vistle::Shm::the().objectID(), vistle::Shm::the().arrayID());
    } catch (const vistle::exception &ex) {
        CERR << ex.what() << endl;
        return false;
    }

    if (!beginExecute()) {
        return false;
    }
    return true;
}

void InSituReader::cancelExecuteMessageReceived(const vistle::message::Message *msg)
{
    if (m_isExecuting) {
        vistle::Shm::the().setArrayID(m_shmIDs.arrayID());
        vistle::Shm::the().setObjectID(m_shmIDs.objectID());
        if (!endExecute()) {
            sendError("failed to prepare reduce");
        }
        assert(m_exec);
        reduceWrapper(m_exec);
        m_isExecuting = false;
    }
}

size_t InSituReader::instanceNum() const
{
    return m_instanceNum;
}

void insitu::InSituReader::reconnect()
{
    initRecvFromSimQueue();
    m_shmIDs.close();
    m_shmIDs.initialize(id(), rank(), std::to_string(m_numInstances), insitu::message::SyncShmIDs::Mode::Create);
    m_instanceNum = m_numInstances;
    ++m_numInstances;
}

vistle::insitu::message::ModuleInfo::ShmInfo insitu::InSituReader::gatherModuleInfo() const
{
    message::ModuleInfo::ShmInfo shmInfo;
    shmInfo.hostname = vistle::hostname();
    shmInfo.id = id();
    shmInfo.mpiSize = size();
    shmInfo.name = name();
    shmInfo.numCons = instanceNum();
    shmInfo.shmName = vistle::Shm::the().instanceName();
    CERR << "vistle::Shm::the().instanceName() = " << vistle::Shm::the().instanceName()
         << " vistle::Shm::the().name() = " << vistle::Shm::the().name() << endl;
    return shmInfo;
}

bool vistle::insitu::InSituReader::sendMessage(const vistle::message::Message &message, const buffer *payload) const
{
    if (payload && m_isExecuting) {
        CERR
            << "InSituReader: can't send message with payload while executing because that would create a vistle-object"
            << endl;
        return false;
    }
    return Module::sendMessage(message, payload);
}

void InSituReader::initRecvFromSimQueue()
{
    std::string msqName = vistle::message::MessageQueue::createName(
        ("recvFromSim" + std::to_string(m_numInstances)).c_str(), id(), rank());
    std::cerr << "created msqName " << msqName << std::endl;
    try {
        m_receiveFromSimMessageQueue.reset(vistle::message::MessageQueue::create(msqName));
        CERR << "receiveFromSimMessageQueue name = " << msqName << std::endl;
    } catch (boost::interprocess::interprocess_exception &ex) {
        throw vistle::exception(std::string("opening recv from sim message queue with name ") + msqName + ": " +
                                ex.what());
    }
}
