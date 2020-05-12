#include "addObjectMsq.h"
#include "exeption.h"
#include <core/messagequeue.h>
#include <core/messages.h>
using namespace insitu;



insitu::AddObjectMsq::AddObjectMsq(const ModuleInfo& moduleInfo, size_t rank)
{
    vistle::message::DefaultSender::init(moduleInfo.id, rank);
    // names are swapped relative to communicator
    std::string mqName = vistle::message::MessageQueue::createName(("recvFromSim" + moduleInfo.numCons).c_str(), moduleInfo.id, rank);
    try {
        m_sendMessageQueue = vistle::message::MessageQueue::open(mqName);
    }
    catch (boost::interprocess::interprocess_exception& ex) {
        throw InsituExeption() << "opening send message queue " << mqName << ": " << ex.what();
    }
}

insitu::AddObjectMsq::~AddObjectMsq()
{
    delete m_sendMessageQueue;
}

insitu::AddObjectMsq::AddObjectMsq(AddObjectMsq&& other)
    :m_sendMessageQueue(other.m_sendMessageQueue)
{
    other.m_sendMessageQueue = nullptr;
}

AddObjectMsq &insitu::AddObjectMsq::operator=(AddObjectMsq&& other)
{
    m_sendMessageQueue = other.m_sendMessageQueue;
    other.m_sendMessageQueue = nullptr;
    return *this;
}

void insitu::AddObjectMsq::addObject(const std::string& port, vistle::Object::const_ptr obj)
{
    vistle::message::AddObject msg(port, obj);
    vistle::message::Buffer buf(msg);

#ifdef MODULE_THREAD
    buf.setSenderId(m_moduleInfo.id);
    buf.setRank(m_rank);
#endif
    m_sendMessageQueue->send(buf);
}
