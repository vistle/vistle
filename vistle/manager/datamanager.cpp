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

bool DataManager::requestArray(const std::string &referrer, const std::string &arrayId, int type, int hub, int rank, const std::function<void()> &handler) {

   std::cerr << "requesting array: " << arrayId << " for " << referrer << std::endl;
   auto it = m_outstandingArrays.find(arrayId);
   if (it != m_outstandingArrays.end()) {
       it->second.push_back(handler);
       return true;
   }

#if 0
   void *ptr= Shm::the().getArrayFromName(arrayId);
   if (ptr) {
      return false;
   }
#endif

   m_outstandingArrays[arrayId].push_back(handler);

   message::RequestObject req(hub, rank, arrayId, type, referrer);
   send(req);

   return true;
}

bool DataManager::requestObject(const message::AddObject &add, const std::string &objId, const std::function<void ()> &handler) {

   Object::const_ptr obj = Shm::the().getObjectFromName(objId);
   if (obj) {
      return false;
   }
   m_outstandingAdds[add].push_back(objId);
   m_outstandingRequests.emplace(objId, add);
   m_outstandingObjects[objId].push_back(handler);
   message::RequestObject req(add, objId);
   send(req);
   return true;
}

bool DataManager::requestObject(const std::string &referrer, const std::string &objId, int type, int hub, int rank, const std::function<void()> &handler) {

   Object::const_ptr obj = Shm::the().getObjectFromName(objId);
   if (obj) {
      return false;
   }
   m_outstandingObjects[objId].push_back(handler);
   message::RequestObject req(hub, rank, objId, referrer);
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
        , m_add(&add)
        , m_hub(Communicator::the().clusterManager().state().getHub(add.senderId()))
        , m_rank(add.rank())
    {
    }

    RemoteFetcher(DataManager *dmgr, const std::string &referrer, int hub, int rank)
        : m_dmgr(dmgr)
        , m_add(nullptr)
        , m_referrer(referrer)
        , m_hub(hub)
        , m_rank(rank)
    {
    }

    virtual void requestArray(const std::string &name, int type, const std::function<void()> &completeCallback) override {
        vassert(!m_add);
        ++m_numRequests;
        m_dmgr->requestArray(m_referrer, name, type, m_hub, m_rank, completeCallback);
    }

    virtual void requestObject(const std::string &name, const std::function<void()> &completeCallback) override {
        vassert(m_add);
        ++m_numRequests;
        m_dmgr->requestObject(*m_add, name, completeCallback);
    }

    DataManager *m_dmgr;
    const message::AddObject *m_add;
    const std::string m_referrer;
    int m_hub, m_rank;
    size_t m_numRequests;
};

struct ArraySaver {

    ArraySaver(const std::string &name, int type, vistle::oarchive &ar): m_ok(false), m_name(name), m_type(type), m_ar(ar) {}

    template<typename T>
    void operator()(T) {
        if (shm_array<T, typename shm<T>::allocator>::typeId() != m_type) {
            return;
        }

        if (m_ok) {
            m_ok = false;
            std::cerr << "ArraySaver: multiple type matches for data array " << m_name << std::endl;
            return;
        }
        auto &arr = Shm::the().getArrayFromName<T>(m_name);
        if (!arr) {
            std::cerr << "ArraySaver: did not find data array " << m_name << std::endl;
            return;
        }
#if 0
        if (arr->type() != m_type) {
            std::cerr << "ArraySaver: " << m_name << " does not have expected type, is " << arr->type() << ", expected " << m_type << std::endl;
            return;
        }
#endif
        m_ar & arr;
        m_ok = true;
    }

    bool m_ok;
    std::string m_name;
    int m_type;
    vistle::oarchive &m_ar;
};

struct ArrayLoader {

    ArrayLoader(const std::string &name, int type, vistle::iarchive &ar): m_ok(false), m_name(name), m_type(type), m_ar(ar) {}

    template<typename T>
    void operator()(T) {
        if (shm_array<T, typename shm<T>::allocator>::typeId() == m_type) {
            if (m_ok) {
                m_ok = false;
                std::cerr << "ArrayLoader: type matches for data array " << m_name << std::endl;
                return;
            }
            auto arr = Shm::the().getArrayFromName<T>(m_name);
            if (arr) {
                std::cerr << "ArrayLoader: have data array with name " << m_name << std::endl;
                return;
            }
            arr = ShmVector<T>();
            m_ar & arr;
#if 0
            if (arr->type() != m_type) {
                std::cerr << "ArrayLoader: " << m_name << " does not have expected type, is " << arr->type() << ", expected " << m_type << std::endl;
                return;
            }
#endif
            m_ok = true;
        }
    }

    bool m_ok;
    std::string m_name;
    int m_type;
    vistle::iarchive &m_ar;
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

   boost::shared_ptr<Fetcher> fetcher;
   std::vector<char> buf(snd.payloadSize());
   read(buf.data(), buf.size());
   vecstreambuf<char> membuf(buf);
   vistle::iarchive memar(membuf);
   if (snd.isArray()) {
       ArrayLoader loader(snd.objectId(), snd.objectType(), memar);
       boost::mpl::for_each<VectorTypes>(loader);
       if (!loader.m_ok) {
           CERR << "failed to restore array " << snd.objectId() << " from archive" << std::endl;
           return false;
       }
       CERR << "restored array " << snd.objectId() << ", dangling in memory" << std::endl;
       auto it = m_outstandingArrays.find(snd.objectId());
       vassert(it != m_outstandingArrays.end());
       if (it != m_outstandingArrays.end()) {
           for (const auto &f: it->second)
               f();
           m_outstandingArrays.erase(it);
       }
       return true;
   } else {
       std::string objName = snd.objectId();
       auto objIt = m_outstandingObjects.find(objName);
       if (objIt == m_outstandingObjects.end()) {
           CERR << "object " << objName << " unexpected" << std::endl;
           return false;
       }

       auto &outstandingAdds = m_outstandingAdds;
       auto &outstandingRequests = m_outstandingRequests;
       auto senderId = snd.senderId();
       auto senderRank = snd.rank();
       auto completionHandler = [this, &outstandingAdds, &outstandingRequests, senderId, senderRank, objName] () mutable -> void {
           auto obj = Shm::the().getObjectFromName(objName);
           if (!obj) {
               std::cerr << "did not receive an object for " << objName << std::endl;
               return;
           }
           vassert(obj);
           std::cerr << "received " << obj->getName() << ", type: " << obj->getType() << ", refcount: " << obj->refcount() << std::endl;
           vassert(obj->check());

           auto reqIt = outstandingRequests.find(objName);
           if (reqIt != outstandingRequests.end())
           {
               message::AddObject &add = reqIt->second;

               auto addIt = outstandingAdds.find(add);
               if (addIt == outstandingAdds.end())
                   return;
               auto &ids = addIt->second;
               auto it = std::find(ids.begin(), ids.end(), objName);
               if (it != ids.end()) {
                   ids.erase(it);
               }
               outstandingRequests.erase(reqIt);

               if (ids.empty()) {
                   message::AddObjectCompleted complete(add);
                   Communicator::the().clusterManager().sendMessage(senderId, complete, senderRank);
                   message::AddObject nadd(add.getSenderPort(), obj);
                   bool ret = Communicator::the().clusterManager().handlePriv(add, /* synthesized = */ true);
                   m_outstandingAdds.erase(addIt);
                   return;
               }
           }
       };
       memar.setObjectCompletionHandler(completionHandler);

       fetcher.reset(new RemoteFetcher(this, snd.referrer(), snd.senderId(), snd.rank()));
       memar.setFetcher(fetcher);
       std::cerr << "loading object from memar" << std::endl;
       Object::ptr obj = Object::load(memar);
       if (obj && obj->isComplete()) {
           return true;
       } else if (obj) {
           obj->ref(); // FIXME
       }
   }

   return true;
}

} // namespace vistle
