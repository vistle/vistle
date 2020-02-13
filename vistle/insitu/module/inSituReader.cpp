#include "inSituReader.h"
#include <core/message.h>

using namespace vistle;
using namespace insitu;
using std::endl;
#define CERR std::cerr << "inSituReader["<< rank() << "/" << size() << "] "

InSituReader::InSituReader(const std::string& description, const std::string& name, const int moduleID, mpi::communicator comm)
    :Module(description, name, moduleID, comm){
    setReducePolicy(message::ReducePolicy::OverAll);
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

void InSituReader::cancelExecuteMessageReceived(const message::Message* msg) {
    if (m_isExecuting) {
        vistle::message::Buffer buf;
        bool finished = false;
        if (!prepareReduce()) {
            sendError("failed to prepare reduce");
            return;
        }
        reduceWrapper(m_exec);
        m_isExecuting = false;
    }
}

