#include <boost/mpl/for_each.hpp>

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

bool DataManager::requestArray(const std::string &arrayId, int type, int hub, int rank) {

   void *ptr= Shm::the().getArrayFromName(arrayId);
   if (ptr) {
      return false;
   }

   message::RequestObject req(hub, rank, arrayId, type, "" /* FIXME */ );
   send(req);

   return true;
}

bool DataManager::requestObject(const message::AddObject &add, const std::string &objId) {

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

class RemoteFetcher: public Fetcher {
public:
    RemoteFetcher(DataManager *dmgr, const message::AddObject &add)
        : m_dmgr(dmgr)
        , m_add(add)
        , m_hub(Communicator::the().clusterManager().state().getHub(add.senderId()))
        , m_rank(add.rank())
    {
    }

    void requestArray(const std::string &name, int type) {
        ++m_numRequests;
        m_dmgr->requestArray(name, type, m_hub, m_rank);
    }

    void requestObject(const std::string &name) {
        ++m_numRequests;
        m_dmgr->requestObject(m_add, name);
    }

    DataManager *m_dmgr;
    message::AddObject m_add;
    int m_hub, m_rank;
    size_t m_numRequests;
};

struct ArraySaver {

    ArraySaver(const std::string &name, int type, vistle::oarchive &ar): m_ok(false), m_name(name), m_type(type), m_ar(ar) {}

    template<typename T>
    void operator()(T) {
        if (ShmVector<T>::typeId() == m_type) {
            if (m_ok) {
                m_ok = false;
                std::cerr << "multiple type matches for data array " << m_name << std::endl;
                return;
            }
            auto *arr = static_cast<ShmVector<T> *>(Shm::the().getArrayFromName(m_name));
            if (!arr) {
                std::cerr << "did not find data array " << m_name << std::endl;
                return;
            }
            if (arr->type() != m_type) {
                std::cerr << "array " << m_name << " doest not have expected type, is " << arr->type() << ", expected " << m_type << std::endl;
                return;
            }
            m_ok = true;
        }
    }

    bool m_ok;
    std::string m_name;
    int m_type;
    vistle::oarchive &m_ar;
};

bool DataManager::handlePriv(const message::RequestObject &req) {
   boost::shared_ptr<message::SendObject> snd;
   vecstreambuf<char> buf;
   const std::vector<char> &mem = buf.get_vector();
   vistle::oarchive memar(buf);
   if (req.isArray()) {
      ArraySaver saver(req.objectId(), req.arrayType(), memar);
      boost::mpl::for_each<VectorTypes>(saver);
      if (!saver.m_ok) {
         CERR << "failed to serialize array " << req.objectId() << std::endl;
         return true;
      }
      snd.reset(new message::SendObject(req, mem.size()));
   } else {
      Object::const_ptr obj = Shm::the().getObjectFromName(req.objectId());
      if (!obj) {
         CERR << "cannot find object with name " << req.objectId() << std::endl;
         return true;
      }
      obj->save(memar);
      snd.reset(new message::SendObject(req, obj, mem.size()));
   }
   snd->setDestId(req.senderId());
   snd->setDestRank(req.rank());
   send(*snd);
   send(mem.data(), mem.size());

   return true;
}

bool DataManager::handlePriv(const message::SendObject &snd) {

   auto reqIt = m_outstandingRequests.find(snd.objectId());
   if (reqIt == m_outstandingRequests.end())
      return false;
   message::AddObject &add = reqIt->second;

   std::vector<char> buf(snd.payloadSize());
   read(buf.data(), buf.size());
   vecstreambuf<char> membuf(buf);
   vistle::iarchive memar(membuf);
   boost::shared_ptr<Fetcher> fetcher(new RemoteFetcher(this, add));
   memar.setFetcher(fetcher);
   Object::ptr obj = Object::load(memar);
   if (obj) {
      CERR << "received " << obj->getName() << ", type: " << obj->getType() << ", refcount: " << obj->refcount() << std::endl;
      vassert(obj->check());
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
