#include "addObjectMsq.h"
#include <vistle/core/messages.h>
#include <vistle/insitu/core/exception.h>
#include <vistle/insitu/message/InSituMessage.h>

using namespace vistle::insitu::message;
using namespace vistle::insitu;

AddObjectMsq::AddObjectMsq(const ModuleInfo &moduleInfo, size_t rank): m_moduleInfo(moduleInfo), m_rank(rank)
{
    vistle::message::DefaultSender::init(moduleInfo.id(), rank);
    // names are swapped relative to communicator
    std::string mqName = vistle::message::MessageQueue::createName(("recvFromSim" + moduleInfo.uniqueSuffix()).c_str(),
                                                                   moduleInfo.id(), rank);
    try {
        m_sendMessageQueue.reset(vistle::message::MessageQueue::open(mqName));
    } catch (boost::interprocess::interprocess_exception &ex) {
        throw vistle::insitu::InsituException()
            << "opening add object message queue with name " << mqName << ": " << ex.what();
    }
}

void AddObjectMsq::addObject(const std::string &port, vistle::Object::const_ptr obj)
{
    vistle::message::AddObject msg(port, obj);
    vistle::message::Buffer buf(msg);

#ifdef MODULE_THREAD
    buf.setSenderId(m_moduleInfo.id());
    buf.setRank(m_rank);
#else
    (void)m_rank;
#endif
    m_cache.push_back(std::move(buf));
}

void AddObjectMsq::sendObjects()
{
    if (m_cache.empty())
        return; //if for some reason some ranks send objects and others not this breaks the sync in the module
    for (auto &buf: m_cache) {
        m_sendMessageQueue->send(std::move(buf));
    }
    m_cache.clear();
    m_sendMessageQueue->send(InSituMessage{InSituMessageType::PackageComplete});
}

int AddObjectMsq::moduleId() const
{
    return m_moduleInfo.id();
}
