#include "addObjectMsq.h"
#include <insitu/core/exeption.h>
#include <core/messagequeue.h>
#include <core/messages.h>
using namespace vistle::insitu::message;
using namespace vistle::insitu;


AddObjectMsq::AddObjectMsq(const ModuleInfo& moduleInfo, size_t rank)
    :m_moduleInfo(moduleInfo)
    , m_rank(rank)
{
    vistle::message::DefaultSender::init(moduleInfo.id, rank);
    // names are swapped relative to communicator
    std::string mqName = vistle::message::MessageQueue::createName(("recvFromSim" + moduleInfo.numCons).c_str(), moduleInfo.id, rank);
    try {
        m_sendMessageQueue = vistle::message::MessageQueue::open(mqName);
    }
    catch (boost::interprocess::interprocess_exception& ex) {
        throw vistle::insitu::InsituExeption() << "opening add object message queue with name " << mqName << ": " << ex.what();
    }
}

AddObjectMsq::~AddObjectMsq()
{
    delete m_sendMessageQueue;
}

AddObjectMsq::AddObjectMsq(AddObjectMsq&& other) noexcept
    :m_sendMessageQueue(other.m_sendMessageQueue)
{
    other.m_sendMessageQueue = nullptr;
}

AddObjectMsq &AddObjectMsq::operator=(AddObjectMsq&& other) noexcept
{
    m_sendMessageQueue = other.m_sendMessageQueue;
    other.m_sendMessageQueue = nullptr;
    return *this;
}

void AddObjectMsq::addObject(const std::string& port, vistle::Object::const_ptr obj)
{
    vistle::message::AddObject msg(port, obj);
    vistle::message::Buffer buf(msg);

#ifdef MODULE_THREAD
    buf.setSenderId(m_moduleInfo.id);
    buf.setRank(m_rank);
#endif
    m_sendMessageQueue->send(buf);
}
