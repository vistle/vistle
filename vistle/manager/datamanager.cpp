#include <boost/mpl/for_each.hpp>
#include <boost/asio/connect.hpp>
#include <boost/mpi/communicator.hpp>

#include "datamanager.h"
#include "clustermanager.h"
#include "communicator.h"
#include <util/vecstreambuf.h>
#include <util/sleep.h>
#include <core/archives.h>
#include <core/archive_loader.h>
#include <core/archive_saver.h>
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
, m_recvThread([this](){ recvLoop(); })
{
    if (m_size > 1)
        m_req = m_comm.irecv(boost::mpi::any_source, Communicator::TagData, &m_msgSize, 1);

}

DataManager::~DataManager() {

    m_quit = true;

    if (m_size > 1)
        m_req.cancel();

    m_recvThread.join();
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
            gotMsg = false;
            message::Buffer buf;
            std::vector<char> payload;
            {
                std::lock_guard<std::mutex> guard(m_recvMutex);
                if (!m_recvQueue.empty()) {
                    gotMsg = true;
                    auto &msg = m_recvQueue.front();
                    buf = msg.buf;
                    payload = std::move(msg.payload);
                    m_recvQueue.pop_front();
                }
            }
            if (gotMsg) {
                handle(buf, &payload);
                work = true;
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
   auto it = m_requestedArrays.find(arrayId);
   if (it != m_requestedArrays.end()) {
       it->second.push_back(handler);
       return true;
   }

   m_requestedArrays[arrayId].push_back(handler);
   message::RequestObject req(hub, rank, arrayId, type, referrer);
   send(req);

   return true;
}

bool DataManager::requestObject(const message::AddObject &add, const std::string &objId, const std::function<void ()> &handler) {

   Object::const_ptr obj = Shm::the().getObjectFromName(objId);
   if (obj) {
      handler();
      return false;
   }

   m_outstandingAdds[objId].insert(add);
   CERR << m_outstandingAdds[objId].size() << " outstanding adds for " << objId << std::endl;

   auto it = m_requestedObjects.find(objId);
   if (it != m_requestedObjects.end()) {
       it->second.completionHandlers.push_back(handler);
       return true;
   }
   m_requestedObjects[objId].completionHandlers.push_back(handler);

   message::RequestObject req(add, objId);
   send(req);

   return true;
}

bool DataManager::requestObject(const std::string &referrer, const std::string &objId, int hub, int rank, const std::function<void()> &handler) {

   Object::const_ptr obj = Shm::the().getObjectFromName(objId);
   if (obj) {
      handler();
      return false;
   }

   auto it = m_requestedObjects.find(objId);
   if (it != m_requestedObjects.end()) {
       it->second.completionHandlers.push_back(handler);
       return true;
   }

   m_requestedObjects[objId].completionHandlers.push_back(handler);
   message::RequestObject req(hub, rank, objId, referrer);
   send(req);
   return true;
}

bool DataManager::prepareTransfer(const message::AddObject &add) {
    CERR << "prepareTransfer: retaining " << add.objectName() << std::endl;
    auto result = m_inTransitObjects.emplace(add);
    if (result.second) {
        result.first->ref();
    }

    updateStatus();

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
   //CERR << "AddObjectCompleted: found request " << *it << " for " << complete << ", still " << m_inTransitObjects.size()-1 << " outstanding" << std::endl;
   m_inTransitObjects.erase(it);

   updateStatus();

   return true;
}

void DataManager::updateStatus() {

    if (m_rank == 0)
        Communicator::the().handleMessage(message::DataTransferState(m_inTransitObjects.size()));
    else
        Communicator::the().forwardToMaster(message::DataTransferState(m_inTransitObjects.size()));
}

bool DataManager::notifyTransferComplete(const message::AddObject &addObj) {

    //CERR << "sending completion notification for " << objName << std::endl;
    message::AddObjectCompleted complete(addObj);
    int hub = Communicator::the().clusterManager().idToHub(addObj.senderId());
    return Communicator::the().clusterManager().sendMessage(hub, complete, addObj.rank());
    //return send(complete);
}

bool DataManager::handle(const message::Message &msg, std::vector<char> *payload)
{
    //CERR << "handle: " << msg << std::endl;
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
    case message::ADDOBJECTCOMPLETED:
        return handlePriv(msg.as<AddObjectCompleted>());
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

    void requestArray(const std::string &name, int type, const std::function<void()> &completeCallback) override {
        vassert(!m_add);
        m_dmgr->requestArray(m_referrer, name, type, m_hub, m_rank, completeCallback);
    }

    void requestObject(const std::string &name, const std::function<void()> &completeCallback) override {
        m_dmgr->requestObject(m_referrer, name, m_hub, m_rank, completeCallback);
    }

    DataManager *m_dmgr;
    const message::AddObject *m_add;
    const std::string m_referrer;
    int m_hub, m_rank;
};

bool DataManager::handlePriv(const message::RequestObject &req) {
   std::shared_ptr<message::SendObject> snd;
   vecostreambuf<char> buf;
   std::vector<char> &mem = buf.get_vector();
   vistle::oarchive memar(buf);
#ifdef USE_YAS
   memar.setCompressionMode(Communicator::the().clusterManager().fieldCompressionMode());
   memar.setZfpRate(Communicator::the().clusterManager().zfpRate());
   memar.setZfpPrecision(Communicator::the().clusterManager().zfpPrecision());
   memar.setZfpAccuracy(Communicator::the().clusterManager().zfpAccuracy());
#endif
   if (req.isArray()) {
      ArraySaver saver(req.objectId(), req.arrayType(), memar);
      if (!saver.save()) {
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
      obj->saveObject(memar);
      snd.reset(new message::SendObject(req, obj, mem.size()));
   }

   std::vector<char> compressed = message::compressPayload(Communicator::the().clusterManager().archiveCompressionMode(), *snd, mem, Communicator::the().clusterManager().archiveCompressionSpeed());

   snd->setDestId(req.senderId());
   snd->setDestRank(req.rank());
   send(*snd, &compressed);
   //CERR << "sent " << snd->payloadSize() << "(" << snd->payloadRawSize() << ") bytes for " << req << " with " << *snd << std::endl;

   return true;
}

bool DataManager::handlePriv(const message::SendObject &snd, std::vector<char> *payload) {

    std::vector<char> uncompressed = decompressPayload(snd, *payload);
    vecistreambuf<char> membuf(uncompressed);

    if (snd.isArray()) {
        // an array was received
        vistle::iarchive memar(membuf);
        ArrayLoader loader(snd.objectId(), snd.objectType(), memar);
        if (!loader.load()) {
            return false;
        }
        //CERR << "restored array " << snd.objectId() << ", dangling in memory" << std::endl;
        auto it = m_requestedArrays.find(snd.objectId());
        vassert(it != m_requestedArrays.end());
        if (it != m_requestedArrays.end()) {
            for (const auto &completionHandler: it->second)
                completionHandler();
            m_requestedArrays.erase(it);
        }

        return true;
    }

    // an object was received
    std::string objName = snd.objectId();
    auto objIt = m_requestedObjects.find(objName);
    if (objIt == m_requestedObjects.end()) {
        CERR << "object " << objName << " unexpected" << std::endl;
        return false;
    }

    auto senderId = snd.senderId();
    auto senderRank = snd.rank();
    auto completionHandler = [this, senderId, senderRank, objName] () mutable -> void {
        auto addIt = m_outstandingAdds.find(objName);
        if (addIt == m_outstandingAdds.end()) {
            // that's normal if a sub-object was loaded
            //CERR << "no outstanding add for " << objName << std::endl;
        } else {
            for (const auto &add: addIt->second) {
                notifyTransferComplete(add);
            }
            m_outstandingAdds.erase(addIt);
        }

        auto objIt = m_requestedObjects.find(objName);
        if (objIt != m_requestedObjects.end()) {
            for (const auto &handler: objIt->second.completionHandlers) {
                handler();
            }
            m_requestedObjects.erase(objIt);
        } else {
            CERR << "no outstanding object for " << objName << std::endl;
        }
    };

    vistle::iarchive memar(membuf);
    memar.setObjectCompletionHandler(completionHandler);
    std::shared_ptr<Fetcher> fetcher(new RemoteFetcher(this, snd.referrer(), snd.senderId(), snd.rank()));
    memar.setFetcher(fetcher);
    objIt->second.obj.reset(Object::loadObject(memar));
    if (objIt->second.obj) {
        CERR << "loading from archive failed for " << objName << std::endl;
    }
    assert(objIt->second.obj);

    return true;
}

bool DataManager::handlePriv(const message::AddObjectCompleted &complete) {
    return completeTransfer(complete);
}

void DataManager::recvLoop()
{
    for (;;)
    {
        bool gotMsg = false;
        if (m_quit)
            break;
        if (m_dataSocket.is_open()) {
            message::Buffer buf;
            std::vector<char> payload;
            if (!message::recv(m_dataSocket, buf, gotMsg, false, &payload)) {
                CERR << "Data communication error" << std::endl;
            } else if (gotMsg) {
                std::lock_guard<std::mutex> guard(m_recvMutex);
                m_recvQueue.emplace_back(buf, payload);
                //CERR << "Data received" << std::endl;
            }
        }
        vistle::adaptive_wait(gotMsg || m_quit, this);
    }
}

DataManager::Msg::Msg(message::Buffer &buf, std::vector<char> &payload)
: buf(buf)
, payload(payload)
{
}

} // namespace vistle
