#include "inSituReader.h"
#include <core/message.h>
using namespace vistle;
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
    CERR << "handleExecute start" << endl;
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
        CERR << "start prepareWrapper" << endl;
        applyDelayedChanges();
        ret &= prepareWrapper(exec);
        if (ret) {
            m_exec = exec;
            m_isExecuting = true;
 
        }
        CERR << "handleExecute end after prepare, ret =  " << ret << endl;
        return ret;
    }

#ifdef DETAILED_PROGRESS
    message::Idle idle;
    idle.setReferrer(exec->uuid());
    idle.setDestId(Id::LocalManager);
    sendMessage(idle);
#endif
    CERR << "handleExecute end without prepare" << endl;
    return ret;
}

void vistle::InSituReader::cancelExecuteMessageReceived(const message::Message* msg) {
    CERR << "cancelExecuteMessageReceived start" << endl;
    if (m_isExecuting) {
        CERR << "cancelExecuteMessageReceived reduce wrapper start" << endl;
        reduceWrapper(m_exec);
        CERR << "cancelExecuteMessageReceived reduce wrapper end, wasCancelRequested = " << wasCancelRequested() << endl;
        if (wasCancelRequested()) {//make sure reduce gets called exactly once afer cancel execute
            
            reduce(-1);
        }
        m_isExecuting = false;
    }
    CERR << "cancelExecuteMessageReceived end" << endl;
}