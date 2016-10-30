#include <boost/mpl/for_each.hpp>
#include <boost/asio/connect.hpp>
#include <boost/mpi/communicator.hpp>

#include "datamanager.h"
#include "clustermanager.h"
#include "communicator.h"
#include <util/vecstreambuf.h>
#include <core/archives.h>
#include <core/archive_loader.h>
#include <core/statetracker.h>
#include <core/object.h>
#include <core/tcpmessage.h>
#include <core/messages.h>
#include <core/shmvector.h>
#include <iostream>
#include <functional>

#define CERR std::cerr << "data [" << m_rank << "/" << m_size << "] "

namespace asio = boost::asio;
namespace mpi = boost::mpi;

namespace vistle {

namespace {

bool isLocal(int id) {

   auto &comm = Communicator::the();
   auto &state = comm.clusterManager().state();
   return (state.getHub(id) == comm.hubId());
}

}

DataManager::DataManager(mpi::communicator &comm)
: m_comm(comm)
, m_rank(comm.rank())
, m_size(comm.size())
, m_dataSocket(m_ioService)
{
    if (m_size > 1)
        m_req = m_comm.irecv(boost::mpi::any_source, Communicator::TagData, &m_msgSize, 1);
}

DataManager::~DataManager() {
    if (m_size > 1)
        m_req.cancel();
}

bool DataManager::connect(asio::ip::tcp::resolver::iterator &hub) {
   bool ret = true;
   boost::system::error_code ec;

   CERR << "connecting to local hub..." << std::endl;

   asio::connect(m_dataSocket, hub, ec);
   if (ec) {
      std::cerr << std::endl;
      CERR << "could not establish bulk data connection on rank " << m_rank << std::endl;
      ret = false;
   } else {
      CERR << "connected to local hub" << std::endl;
   }

   return ret;
}

bool DataManager::dispatch() {

   bool work = false;
   m_ioService.poll();
   if (m_dataSocket.is_open()) {
      bool gotMsg = false;
      do {
         message::Buffer buf;
         std::vector<char> payload;
         if (!message::recv(m_dataSocket, buf, gotMsg, false, &payload)) {
            CERR << "Data communication error" << std::endl;
         } else if (gotMsg) {
            work = true;
            //CERR << "Data received" << std::endl;
            handle(buf, &payload);
         } else if (m_size > 1) {
             if (auto status = m_req.test()) {
                 if (!status->cancelled()) {
                     vassert(status->tag() == Communicator::TagData);
                     m_comm.recv(status->source(), Communicator::TagData, buf.data(), m_msgSize);
                     if (buf.payloadSize() > 0) {
                         payload.resize(buf.payloadSize());
                         m_comm.recv(status->source(), Communicator::TagData, payload.data(), buf.payloadSize());
                     }
                     work = true;
                     gotMsg = true;
                     handle(buf, &payload);
                     m_req = m_comm.irecv(mpi::any_source, Communicator::TagData, &m_msgSize, 1);
                 }
             }
         }
      } while(gotMsg);
   }

    return work;
}

bool DataManager::send(const message::Message &message, const std::vector<char> *payload) {

   if (isLocal(message.destId())) {
       const int sz = message.size();
       m_comm.send(message.destRank(), Communicator::TagData, sz);
       m_comm.send(message.destRank(), Communicator::TagData, (const char *)&message, sz);
       if (payload && payload->size() > 0) {
           m_comm.send(message.destRank(), Communicator::TagData, payload->data(), payload->size());
       }
       return true;
   } else {
       return message::send(m_dataSocket, message, payload);
   }
}

bool DataManager::requestArray(const std::string &referrer, const std::string &arrayId, int type, int hub, int rank, const std::function<void()> &handler) {

   //CERR << "requesting array: " << arrayId << " for " << referrer << std::endl;
   auto it = m_outstandingArrays.find(arrayId);
   if (it != m_outstandingArrays.end()) {
       it->second.push_back(handler);
       return true;
   }

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
   auto it = m_outstandingObjects.find(objId);
   if (it != m_outstandingObjects.end()) {
       it->second.completionHandlers.push_back(handler);
       return true;
   }
   m_outstandingObjects[objId].completionHandlers.push_back(handler);
   m_outstandingAdds[add].push_back(objId);
   m_outstandingRequests.emplace(objId, add);
   message::RequestObject req(add, objId);
   send(req);
   return true;
}

bool DataManager::requestObject(const std::string &referrer, const std::string &objId, int hub, int rank, const std::function<void()> &handler) {

   Object::const_ptr obj = Shm::the().getObjectFromName(objId);
   if (obj) {
      return false;
   }
   auto it = m_outstandingObjects.find(objId);
   if (it != m_outstandingObjects.end()) {
       it->second.completionHandlers.push_back(handler);
       return true;
   }

   m_outstandingObjects[objId].completionHandlers.push_back(handler);
   message::RequestObject req(hub, rank, objId, referrer);
   send(req);
   return true;
}

bool DataManager::prepareTransfer(const message::AddObject &add) {
    auto result =  m_inTransitObjects.emplace(add);
    if (result.second) {
        result.first->ref();
    }
    return true;
}

bool DataManager::completeTransfer(const message::AddObjectCompleted &complete) {

   message::AddObject key(complete);
   auto it = m_inTransitObjects.find(key);
   if (it == m_inTransitObjects.end()) {
      CERR << "AddObject message for completion notification not found: " << complete << ", still " << m_inTransitObjects.size() << " objects in transit" << std::endl;
      return true;
   }
   it->takeObject();
   CERR << "AddObjectCompleted: found request " << *it << " for " << complete << ", still " << m_inTransitObjects.size()-1 << " outstanding" << std::endl;
   m_inTransitObjects.erase(it);
   return true;
}

bool DataManager::handle(const message::Message &msg, const std::vector<char> *payload)
{
    CERR << "handle: " << msg << std::endl;
    using namespace message;

    switch (msg.type()) {
    case message::IDENTIFY: {
        auto &mm = static_cast<const Identify &>(msg);
        if (mm.identity() == Identify::REQUEST) {
            return send(Identify(Identify::LOCALBULKDATA, m_rank));
        }
        return true;
    }
    case message::REQUESTOBJECT:
        return handlePriv(static_cast<const RequestObject &>(msg));
    case message::SENDOBJECT:
        return handlePriv(static_cast<const SendObject &>(msg), payload);
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
        ++m_numRequests;
        if (m_add) {
           m_dmgr->requestObject(*m_add, name, completeCallback);
        } else {
           m_dmgr->requestObject(m_referrer, name, m_hub, m_rank, completeCallback);
        }
    }

    DataManager *m_dmgr;
    const message::AddObject *m_add;
    const std::string m_referrer;
    int m_hub, m_rank;
    size_t m_numRequests;
};

struct ArraySaver {

    ArraySaver(const std::string &name, int type, vistle::shallow_oarchive &ar): m_ok(false), m_name(name), m_type(type), m_ar(ar) {}
    ArraySaver() = delete;
    ArraySaver(const ArraySaver &other) = delete;

    template<typename T>
    void operator()(T) {
        if (shm_array<T, typename shm<T>::allocator>::typeId() != m_type) {
            //std::cerr << "ArraySaver: type mismatch - looking for " << m_type << ", is " << shm_array<T, typename shm<T>::allocator>::typeId() << std::endl;
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
        m_ar & m_name;
        m_ar & *arr;
        m_ok = true;
    }

    bool m_ok;
    std::string m_name;
    int m_type;
    vistle::shallow_oarchive &m_ar;
};

bool DataManager::handlePriv(const message::RequestObject &req) {
   std::shared_ptr<message::SendObject> snd;
   vecostreambuf<char> buf;
   const std::vector<char> &mem = buf.get_vector();
   vistle::shallow_oarchive memar(buf);
   if (req.isArray()) {
      ArraySaver saver(req.objectId(), req.arrayType(), memar);
      boost::mpl::for_each<VectorTypes>(std::reference_wrapper<ArraySaver>(saver));
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
   send(*snd, &mem);
   CERR << "sent " << mem.size() << " bytes for " << req << " with " << *snd << std::endl;

   return true;
}

bool DataManager::handlePriv(const message::SendObject &snd, const std::vector<char> *payload) {

   vecistreambuf<char> membuf(*payload);
   if (snd.isArray()) {
       vistle::shallow_iarchive memar(membuf);
       ArrayLoader loader(snd.objectId(), snd.objectType(), memar);
       if (!loader.load()) {
           return false;
       }
       //CERR << "restored array " << snd.objectId() << ", dangling in memory" << std::endl;
       auto it = m_outstandingArrays.find(snd.objectId());
       vassert(it != m_outstandingArrays.end());
       if (it != m_outstandingArrays.end()) {
           for (const auto &completionHandler: it->second)
               completionHandler();
           m_outstandingArrays.erase(it);
       }

       return true;
   } else {
       vistle::shallow_iarchive memar(membuf);
       std::string objName = snd.objectId();
       auto objIt = m_outstandingObjects.find(objName);
       if (objIt == m_outstandingObjects.end()) {
           CERR << "object " << objName << " unexpected" << std::endl;
           return false;
       }

       auto &outstandingAdds = m_outstandingAdds;
       auto &outstandingRequests = m_outstandingRequests;
       auto &outstandingObjects = m_outstandingObjects;
       auto senderId = snd.senderId();
       auto senderRank = snd.rank();
       auto completionHandler = [this, &outstandingAdds, &outstandingRequests, &outstandingObjects, senderId, senderRank, objName] () mutable -> void {
           //CERR << "object completion handler for " << objName << std::endl;
           auto obj = Shm::the().getObjectFromName(objName);
           if (!obj) {
               CERR << "did not receive an object for " << objName << std::endl;
               return;
           }
           vassert(obj);
           CERR << "received " << obj->getName() << ", type: " << obj->getType() << ", refcount: " << obj->refcount() << std::endl;
           vassert(obj->check());

           auto reqIt = outstandingRequests.find(objName);
           if (reqIt != outstandingRequests.end()) {
               message::AddObject &add = reqIt->second;

               auto addIt = outstandingAdds.find(add);
               if (addIt == outstandingAdds.end()) {
                   CERR << "no outstanding add for " << objName << std::endl;
                   return;
               }
               auto &ids = addIt->second;
               auto it = std::find(ids.begin(), ids.end(), objName);
               if (it != ids.end()) {
                   ids.erase(it);
               }

               if (ids.empty()) {
                   CERR << "sending completion notification for " << objName << std::endl;
                   message::AddObjectCompleted complete(add);
                   Communicator::the().clusterManager().sendMessage(senderId, complete, senderRank);
                   //message::AddObject nadd(add.getSenderPort(), obj);
                   Communicator::the().clusterManager().handlePriv(add, /* synthesized = */ true);
                   m_outstandingAdds.erase(addIt);
               }
               outstandingRequests.erase(reqIt);
           } else {
               //CERR << "no outstanding request for " << obj->getName() << std::endl;
           }

           auto objIt = outstandingObjects.find(objName);
           if (objIt != outstandingObjects.end()) {
               for (const auto &handler: objIt->second.completionHandlers) {
                   handler();
               }
               objIt->second.obj->unref();
               outstandingObjects.erase(objIt);
               //CERR << "erasing from outstanding objects: " << obj->getName() << std::endl;
           } else {
               CERR << "no outstanding object for " << obj->getName() << std::endl;
           }
       };
       memar.setObjectCompletionHandler(completionHandler);

       std::shared_ptr<Fetcher> fetcher(new RemoteFetcher(this, snd.referrer(), snd.senderId(), snd.rank()));
       memar.setFetcher(fetcher);
       //CERR << "loading object " << objName << " from memar" << std::endl;
       objIt->second.obj = Object::load(memar);
       objIt->second.obj->ref();
   }

   return true;
}

} // namespace vistle
