#include "datamanager.h"
#include "clustermanager.h"
#include "communicator.h"
#include <util/vecstreambuf.h>
#include <core/archives.h>
#include <core/statetracker.h>
#include <core/object.h>
#include <core/tcpmessage.h>
#include <iostream>

#define CERR std::cerr << "data [" << m_rank << "/" << m_size << "] "

namespace asio = boost::asio;

namespace vistle {

DataManager::DataManager(int rank, int size)
: m_rank(rank)
, m_size(size)
, m_dataSocket(m_ioService)
{
}

bool DataManager::connect(asio::ip::tcp::resolver::iterator &hub) {
   bool ret = true;
   boost::system::error_code ec;

   asio::connect(m_dataSocket, hub, ec);
   if (ec) {
      std::cerr << std::endl;
      CERR << "could not establish bulk data connection on rank " << m_rank << std::endl;
      ret = false;
   }

   return ret;
}

bool DataManager::dispatch() {

   m_ioService.poll();
   if (m_dataSocket.is_open()) {
      message::Buffer buf;
      bool gotMsg = false;
      boost::lock_guard<boost::mutex> lock(m_dataReadMutex);
      if (!message::recv(m_dataSocket, buf, gotMsg)) {
         CERR << "Data communication error" << std::endl;
      } else if (gotMsg) {
         CERR << "Data received" << std::endl;
         handle(buf);
      }
   }

    return true;
}

bool DataManager::send(const message::Message &message) {

   return message::send(m_dataSocket, message);
}

bool DataManager::send(const char *buf, size_t n) {

   return asio::write(m_dataSocket, asio::buffer(buf, n));
}

bool DataManager::read(char *buf, size_t n) {
   return asio::read(m_dataSocket, asio::buffer(buf, n));
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
      send(req);
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
    CERR << "handle: " << msg << std::endl;
    using namespace message;

    switch (msg.type()) {
    case Message::IDENTIFY: {
       return send(Identify(Identify::LOCALBULKDATA, m_rank));
    }
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
   message::SendObject snd(req, obj, mem.size());
   snd.setDestId(req.senderId());
   snd.setDestRank(req.rank());
   send(snd);
   send(mem.data(), mem.size());

   return true;
}

bool DataManager::handlePriv(const message::SendObject &snd) {

   std::vector<char> buf(snd.payloadSize());
   read(buf.data(), buf.size());
   vecstreambuf<char> membuf(buf);
   vistle::iarchive memar(membuf);
   memar.setSource(Communicator::the().clusterManager().state().getHub(snd.senderId()), snd.rank());
   Object::ptr obj = Object::load(memar);
   if (obj) {
      CERR << "received " << obj->getName() << ", type: " << obj->getType() << ", refcount: " << obj->refcount() << std::endl;
      vassert(obj->check());
      auto reqIt = m_outstandingRequests.find(snd.objectId());
      if (reqIt == m_outstandingRequests.end())
         return false;
      message::AddObject &add = reqIt->second;
      auto addIt = m_outstandingAdds.find(add);
      if (addIt == m_outstandingAdds.end())
         return false;
      auto &ids = addIt->second;
      auto it = std::find(ids.begin(), ids.end(), snd.objectId());
      if (it != ids.end()) {
         ids.erase(it);
      }
      m_outstandingRequests.erase(reqIt);
      if (ids.empty()) {
         message::AddObjectCompleted complete(add);
         Communicator::the().clusterManager().sendMessage(snd.senderId(), complete, snd.rank());
         message::AddObject nadd(add.getSenderPort(), obj);
         bool ret = Communicator::the().clusterManager().handlePriv(add, /* synthesized = */ true);
         m_outstandingAdds.erase(addIt);
         return ret;
      }
      return true;
   }

   return true;
}

} // namespace vistle
