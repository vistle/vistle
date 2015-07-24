#include "datamanager.h"
#include "clustermanager.h"
#include "communicator.h"
#include <util/vecstreambuf.h>
#include <core/archives.h>
#include <core/object.h>
#include <iostream>

#define CERR std::cerr << "data [" << m_comm.getRank() << "/" << m_comm.getSize() << "] "

namespace vistle {

DataManager::DataManager(vistle::Communicator &comm)
: m_comm(comm)
{
}

bool DataManager::requestObject(const message::AddObject &add, const std::string &objId, bool array) {

   if (array) {
      return false;
   } else {
      Object::const_ptr obj = Shm::the().getObjectFromName(objId);
      if (obj) {
         return false;
      }
      m_outstandingAdds[add].push_back(objId);
      m_outstandingRequests.emplace(objId, add);
      message::RequestObject req(add, objId);
      Communicator::the().sendData(req);
      return true;
   }
}

bool DataManager::prepareTransfer(const message::AddObject &add) {
    m_inTransitObjects.emplace(add);
    return true;
}

bool DataManager::completeTransfer(const message::AddObjectCompleted &complete) {

   message::AddObject key(complete.originalSenderPort(), nullptr);
   key.setUuid(complete.uuid());
   key.setDestId(complete.destId());
   auto it = m_inTransitObjects.find(key);
   if (it == m_inTransitObjects.end()) {
      CERR << "AddObject message for completion notification not found: " << complete << std::endl;
      return false;
   }
   return true;
}


bool DataManager::handle(const message::Message &msg)
{
    using namespace message;

    switch (msg.type()) {
    case Message::REQUESTOBJECT:
        return handlePriv(static_cast<const RequestObject &>(msg));
    case Message::SENDOBJECT:
        return handlePriv(static_cast<const SendObject &>(msg));
    default:
        break;
    }

    CERR << "invalid message type " << msg.type() << std::endl;
    return false;
}

bool DataManager::handlePriv(const message::RequestObject &req) {
   Object::const_ptr obj = Shm::the().getObjectFromName(req.objectId());
   if (!obj) {
      CERR << "cannot find object with name " << req.objectId() << std::endl;
      return true;
   }
   vecstreambuf<char> buf;
   vistle::oarchive memar(buf);
   obj->save(memar);
   const std::vector<char> &mem = buf.get_vector();
   message::SendObject send(req, obj, mem.size());
   send.setDestId(req.senderId());
   send.setDestRank(req.rank());
   Communicator::the().sendData(send);
   Communicator::the().sendData(mem.data(), mem.size());

   return true;
}

bool DataManager::handlePriv(const message::SendObject &send) {

   std::vector<char> buf(send.payloadSize());
   Communicator::the().readData(buf.data(), buf.size());
   vecstreambuf<char> membuf(buf);
   vistle::iarchive memar(membuf);
   memar.setSource(m_comm.clusterManager().state().getHub(send.senderId()), send.rank());
   Object::ptr obj = Object::load(memar);
   if (obj) {
      CERR << "received " << obj->getName() << ", type: " << obj->getType() << ", refcount: " << obj->refcount() << std::endl;
      vassert(obj->check());
      auto reqIt = m_outstandingRequests.find(send.objectId());
      if (reqIt == m_outstandingRequests.end())
         return false;
      message::AddObject &add = reqIt->second;
      auto addIt = m_outstandingAdds.find(add);
      if (addIt == m_outstandingAdds.end())
         return false;
      auto &ids = addIt->second;
      auto it = std::find(ids.begin(), ids.end(), send.objectId());
      if (it != ids.end()) {
         ids.erase(it);
      }
      m_outstandingRequests.erase(reqIt);
      if (ids.empty()) {
         message::AddObjectCompleted complete(add);
         m_comm.clusterManager().sendMessage(send.senderId(), complete, send.rank());
         message::AddObject nadd(add.getSenderPort(), obj);
         bool ret = m_comm.clusterManager().handlePriv(add, /* synthesized = */ true);
         m_outstandingAdds.erase(addIt);
         return ret;
      }
      return true;
   }

   return true;
}

} // namespace vistle
