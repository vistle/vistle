#include "inSituReader.h"
#include <core/message.h>
#include <core/messagequeue.h>
using namespace vistle;
using std::endl;
#define CERR std::cerr << "inSituReader["<< rank() << "/" << size() << "] "

InSituReader::InSituReader(const std::string& description, const std::string& name, const int moduleID, mpi::communicator comm)
    :Module(description, name, moduleID, comm){
    setReducePolicy(message::ReducePolicy::OverAll);
    std::string rmqName = message::MessageQueue::createName("recvFromSim", id(), rank());
    try {
        m_receiveFromSimMessageQueue = message::MessageQueue::create(rmqName);
    } catch (interprocess::interprocess_exception & ex) {
        throw vistle::exception(std::string("opening receive message queue ") + rmqName + ": " + ex.what());
    }
}

vistle::InSituReader::~InSituReader() {
    delete m_receiveFromSimMessageQueue;
}

bool InSituReader::isExecuting() {
    return m_isExecuting;
}

bool vistle::InSituReader::dispatch(bool block, bool* messageReceived) {
    
    vistle::message::Buffer buf;
    while (m_receiveFromSimMessageQueue->tryReceive(buf)) {
        if (buf.type() != vistle::message::SYNCSHMIDS) {
            sendMessage(buf);
        }
    }
    
    return Module::dispatch(block, messageReceived);
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

void vistle::InSituReader::cancelExecuteMessageReceived(const message::Message* msg) {
    if (m_isExecuting) {
        vistle::message::Buffer buf;
        bool finished = false;
        if (!prepareReduce()) {
            sendError("failed to prepare reduce");
            return;
        }
        while (!finished) {
            m_receiveFromSimMessageQueue->receive(buf);
            if (buf.type() == vistle::message::SYNCSHMIDS) {
                auto msg = buf.as<vistle::message::SyncShmIDs>();
                Shm::the().setArrayID(msg.arrayID());
                Shm::the().setObjectID(msg.objectID());
                finished = true;
            } else {
                sendMessage(buf);
            }

        }
        if (reduceWrapper(m_exec)) {
            sendError("failed to reduce");
            return;
        }
        if (wasCancelRequested()) {//make sure reduce gets called exactly once afer cancel execute
            
            if (reduce(-1)) {
                sendError("failed to reduce");
                return;
            }
        }
        m_isExecuting = false;
    }
}

