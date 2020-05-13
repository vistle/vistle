#include "inSituReader.h"
#include <core/message.h>
#include <util/sleep.h>

using namespace vistle;
using namespace insitu;
using std::endl;
#define CERR std::cerr << "inSituReader["<< rank() << "/" << size() << "] "

size_t InSituReader::m_numInstances = 0;

InSituReader::InSituReader(const std::string& description, const std::string& name, const int moduleID, mpi::communicator comm)
    :Module(description, name, moduleID, comm){
    setReducePolicy(vistle::message::ReducePolicy::OverAll);
}



bool InSituReader::isExecuting() {
    return m_isExecuting;
}

bool InSituReader::handleExecute(const vistle::message::Execute* exec) {
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
    if (!m_isExecuting &&(exec->what() == Execute::ComputeExecute
        || exec->what() == Execute::Prepare)) {
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

bool InSituReader::dispatch(bool block, bool* messageReceived) {
    bool passMsg = false;
    bool msgRecv = false;
    vistle::message::Buffer buf;
    if (m_receiveFromSimMessageQueue && m_receiveFromSimMessageQueue->tryReceive(buf)) {
        if (buf.type() != vistle::message::INSITU) {
            sendMessage(buf);
        }
        passMsg = true;
    }
    bool retval = Module::dispatch(false, &msgRecv);
    vistle::adaptive_wait((msgRecv || passMsg));
    if (messageReceived) {
        *messageReceived = msgRecv;
    }
    return retval;

}

bool InSituReader::prepare() {

    try {
        m_shmIDs.set(vistle::Shm::the().objectID(), vistle::Shm::the().arrayID());
    }
    catch (const vistle::exception& ex) {
        CERR << ex.what() << endl;
        return false;
    }

    if (!beginExecute())
    {
        return false;
    }
    return true;
}

void InSituReader::cancelExecuteMessageReceived(const vistle::message::Message* msg) {
    if (m_isExecuting) {
        bool finished = false;
        vistle::Shm::the().setArrayID(m_shmIDs.arrayID());
        vistle::Shm::the().setObjectID(m_shmIDs.objectID());
        if (!endExecute()) {
            sendError("failed to prepare reduce");
            return;
        }
        reduceWrapper(m_exec);
        m_isExecuting = false;
    }
}

size_t InSituReader::InstanceNum() const {
    return m_instanceNum;
}

void insitu::InSituReader::reconnect()
{
    initRecvFromSimQueue();
    m_shmIDs.initialize(id(), rank(), m_numInstances, insitu::message::SyncShmIDs::Mode::Create);
    m_instanceNum = m_numInstances;
    ++m_numInstances;
}

void InSituReader::initRecvFromSimQueue() {
    std::string msqName = vistle::message::MessageQueue::createName(("recvFromSim" + std::to_string(m_numInstances)).c_str(), id(), rank());

    try {
        m_receiveFromSimMessageQueue.reset(vistle::message::MessageQueue::create(msqName));
        CERR << "receiveFromSimMessageQueue name = " << msqName << std::endl;
    }
    catch (boost::interprocess::interprocess_exception& ex) {
        throw vistle::exception(std::string("opening send message queue ") + msqName + ": " + ex.what());

    }
}

