#include <boost/mpl/for_each.hpp>
#include <boost/asio/connect.hpp>
#include <boost/mpi/communicator.hpp>

#include "datamanager.h"
#include "clustermanager.h"
#include "communicator.h"
#include <vistle/util/vecstreambuf.h>
#include <vistle/util/sleep.h>
#include <vistle/core/archives.h>
#include <vistle/core/archive_loader.h>
#include <vistle/core/archive_saver.h>
#include <vistle/core/statetracker.h>
#include <vistle/core/object.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/core/messages.h>
#include <vistle/core/shmvector.h>
#include <iostream>
#include <functional>

#define CERR std::cerr << "data [" << m_rank << "/" << m_size << "] "

//#define DEBUG

// don't wait for message sends to complete
#define ASYNC_SEND

namespace asio = boost::asio;
namespace mpi = boost::mpi;

namespace vistle {

namespace {

bool isLocal(int id)
{
    auto &comm = Communicator::the();
    auto &state = comm.clusterManager().state();
    return (state.getHub(id) == comm.hubId());
}

} // namespace

DataManager::DataManager(mpi::communicator &comm)
: m_comm(comm, mpi::comm_duplicate)
, m_rank(m_comm.rank())
, m_size(m_comm.size())
, m_dataSocket(m_ioService)
#if BOOST_VERSION >= 106600
, m_workGuard(asio::make_work_guard(m_ioService))
#else
, m_workGuard(new asio::io_service::work(m_ioService))
#endif
, m_ioThread([this]() { sendLoop(); })
, m_recvThread([this]() { recvLoop(); })
, m_cleanThread([this]() { cleanLoop(); })
{
    if (m_size > 1)
        m_req = m_comm.irecv(boost::mpi::any_source, Communicator::TagData, &m_msgSize, 1);
}

DataManager::~DataManager()
{
    {
        std::lock_guard<std::mutex> guard(m_recvMutex);
        m_quit = true;
    }

    if (m_size > 1)
        m_req.cancel();
    m_req.wait();

    m_recvThread.join();

    m_cleanThread.join();

    m_workGuard.reset();
    m_ioService.stop();
    m_ioThread.join();
}

bool DataManager::connect(asio::ip::tcp::resolver::iterator &hub)
{
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

bool DataManager::dispatch()
{
    bool work = false;
    for (bool gotMsg = true; m_dataSocket.is_open() && gotMsg;) {
        gotMsg = false;
        message::Buffer buf;
        buffer payload;
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
            continue;
        }

        if (m_size <= 1) {
            continue;
        }
        std::unique_lock<Communicator> guard(Communicator::the());
        auto status = m_req.test();
        if (status && !status->cancelled()) {
            assert(status->tag() == Communicator::TagData);
            m_comm.recv(status->source(), Communicator::TagData, buf.data(), m_msgSize);
            if (buf.payloadSize() > 0) {
                payload.resize(buf.payloadSize());
                m_comm.recv(status->source(), Communicator::TagData, payload.data(), buf.payloadSize());
            }
            guard.unlock();
            work = true;
            gotMsg = true;
            handle(buf, &payload);
            guard.lock();
            m_req = m_comm.irecv(mpi::any_source, Communicator::TagData, &m_msgSize, 1);
        }
    }

    return work;
}

void DataManager::trace(message::Type type)
{
    m_traceMessages = type;
}

bool DataManager::send(const message::Message &message, std::shared_ptr<buffer> payload)
{
    if (isLocal(message.destId())) {
        std::unique_lock<Communicator> guard(Communicator::the());
        const int sz = message.size();
        m_comm.send(message.destRank(), Communicator::TagData, sz);
        m_comm.send(message.destRank(), Communicator::TagData, (const char *)&message, sz);
        if (payload && payload->size() > 0) {
            m_comm.send(message.destRank(), Communicator::TagData, payload->data(), payload->size());
        }
        return true;
    } else {
#ifdef ASYNC_SEND
        //CERR << "async send: " << message << std::endl;
        message::async_send(m_dataSocket, message, payload, [this](boost::system::error_code ec) {
            if (ec) {
                CERR << "ERROR: async send to " << m_dataSocket.remote_endpoint() << " failed: " << ec.message()
                     << std::endl;
            }
        });
        return true;
#else
        return message::send(m_dataSocket, message, payload.get());
#endif
    }
}

bool DataManager::requestArray(const std::string &referrer, const std::string &arrayId, int type, int hub, int rank,
                               const ArrayCompletionHandler &handler)
{
    //CERR << "requesting array: " << arrayId << " for " << referrer << std::endl;
    {
        std::lock_guard<std::mutex> lock(m_requestArrayMutex);
        auto it = m_requestedArrays.find(arrayId);
        if (it != m_requestedArrays.end()) {
            it->second.push_back(handler);
#ifdef DEBUG
            CERR << "requesting array: " << arrayId << " for " << referrer << ", piggybacking..." << std::endl;
#endif
            return true;
        }
#ifdef DEBUG
        CERR << "requesting array: " << arrayId << " for " << referrer << ", requesting..." << std::endl;
#endif
        m_requestedArrays[arrayId].push_back(handler);
    }

    message::RequestObject req(hub, rank, arrayId, type, referrer);
    send(req);
    return true;
}

bool DataManager::requestObject(const message::AddObject &add, const std::string &objId,
                                const ObjectCompletionHandler &handler)
{
    Object::const_ptr obj = Shm::the().getObjectFromName(objId);
    if (obj) {
#ifdef DEBUG
        std::lock_guard<std::mutex> lock(m_requestObjectMutex);
        CERR << m_outstandingAdds[objId].size() << " outstanding adds for " << objId << ", already have!" << std::endl;
        lock.unlock();
#endif
        handler(obj);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_requestObjectMutex);
        m_outstandingAdds[objId].insert(add);

        auto it = m_requestedObjects.find(objId);
        if (it != m_requestedObjects.end()) {
            it->second.completionHandlers.push_back(handler);
#ifdef DEBUG
            CERR << m_outstandingAdds[objId].size() << " outstanding adds for " << objId << ", piggybacking..."
                 << std::endl;
#endif
            return true;
        }
        m_requestedObjects[objId].completionHandlers.push_back(handler);
#ifdef DEBUG
        CERR << m_outstandingAdds[objId].size() << " outstanding adds for " << objId << ", requesting..." << std::endl;
#endif
    }

    message::RequestObject req(add, objId);
    send(req);
    return true;
}

bool DataManager::requestObject(const std::string &referrer, const std::string &objId, int hub, int rank,
                                const ObjectCompletionHandler &handler)
{
    Object::const_ptr obj = Shm::the().getObjectFromName(objId);
    if (obj) {
#ifdef DEBUG
        std::lock_guard<std::mutex> lock(m_requestObjectMutex);
        CERR << m_outstandingAdds[objId].size() << " outstanding adds for subobj " << objId << ", already have!"
             << std::endl;
        lock.unlock();
#endif
        handler(obj);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_requestObjectMutex);
        auto it = m_requestedObjects.find(objId);
        if (it != m_requestedObjects.end()) {
            it->second.completionHandlers.push_back(handler);
#ifdef DEBUG
            CERR << m_outstandingAdds[objId].size() << " outstanding adds for subobj " << objId << ", piggybacking..."
                 << std::endl;
#endif
            return true;
        }

        m_requestedObjects[objId].completionHandlers.push_back(handler);
#ifdef DEBUG
        CERR << m_outstandingAdds[objId].size() << " outstanding adds for subobj " << objId << ", requesting..."
             << std::endl;
#endif
    }

    message::RequestObject req(hub, rank, objId, referrer);
    send(req);
    return true;
}

bool DataManager::prepareTransfer(const message::AddObject &add)
{
#ifdef DEBUG
    CERR << "prepareTransfer: retaining " << add.objectName() << std::endl;
#endif
    auto result = m_inTransitObjects.emplace(add);
    if (result.second) {
        result.first->ref();
    }

    updateStatus();

    return true;
}

bool DataManager::completeTransfer(const message::AddObjectCompleted &complete)
{
    message::AddObject key(complete);
    auto it = m_inTransitObjects.find(key);
    if (it == m_inTransitObjects.end()) {
        CERR << "AddObject message for completion notification not found: " << complete << ", still "
             << m_inTransitObjects.size() << " objects in transit" << std::endl;
        return true;
    }
    it->takeObject();
    //CERR << "AddObjectCompleted: found request " << *it << " for " << complete << ", still " << m_inTransitObjects.size()-1 << " outstanding" << std::endl;
    m_inTransitObjects.erase(it);

    updateStatus();

    return true;
}

void DataManager::updateStatus()
{
    std::unique_lock<Communicator> guard(Communicator::the());

    if (m_rank == 0)
        Communicator::the().handleMessage(message::DataTransferState(m_inTransitObjects.size()));
    else
        Communicator::the().forwardToMaster(message::DataTransferState(m_inTransitObjects.size()));
}

bool DataManager::notifyTransferComplete(const message::AddObject &addObj)
{
    std::unique_lock<Communicator> guard(Communicator::the());

    //CERR << "sending completion notification for " << objName << std::endl;
    message::AddObjectCompleted complete(addObj);
    int hub = Communicator::the().clusterManager().idToHub(addObj.senderId());
    return Communicator::the().clusterManager().sendMessage(hub, complete, addObj.rank());
    //return send(complete);
}

bool DataManager::handle(const message::Message &msg, buffer *payload)
{
    //CERR << "handle: " << msg << std::endl;
    using namespace message;

    if (m_traceMessages == message::ANY || msg.type() == m_traceMessages) {
        CERR << "handle: " << msg << std::endl;
    }

    switch (msg.type()) {
    case message::IDENTIFY: {
        auto &mm = static_cast<const Identify &>(msg);
        if (mm.identity() == Identify::REQUEST) {
            return send(Identify(mm, Identify::LOCALBULKDATA, m_rank));
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
    {}

    RemoteFetcher(DataManager *dmgr, const std::string &referrer, int hub, int rank)
    : m_dmgr(dmgr), m_add(nullptr), m_referrer(referrer), m_hub(hub), m_rank(rank)
    {}

    void requestArray(const std::string &name, int type, const ArrayCompletionHandler &completeCallback) override
    {
        assert(!m_add);
        m_dmgr->requestArray(m_referrer, name, type, m_hub, m_rank, completeCallback);
    }

    void requestObject(const std::string &name, const ObjectCompletionHandler &completeCallback) override
    {
        m_dmgr->requestObject(m_referrer, name, m_hub, m_rank, completeCallback);
    }

    DataManager *m_dmgr;
    const message::AddObject *m_add;
    const std::string m_referrer;
    int m_hub, m_rank;
};

bool DataManager::handlePriv(const message::RequestObject &req)
{
#ifdef DEBUG
    if (req.isArray()) {
        CERR << "request for array " << req.objectId() << std::endl;
    } else {
        CERR << "request for object " << req.objectId() << std::endl;
    }
#endif

    auto fut = std::async(std::launch::async, [this, req]() {
        std::shared_ptr<message::SendObject> snd;
        vecostreambuf<buffer> buf;
        buffer &mem = buf.get_vector();
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
                return false;
            }
            snd.reset(new message::SendObject(req, mem.size()));
        } else {
            Object::const_ptr obj = Shm::the().getObjectFromName(req.objectId());
            if (!obj) {
                CERR << "cannot find object with name " << req.objectId() << std::endl;
                return false;
            }
            obj->saveObject(memar);
            snd.reset(new message::SendObject(req, obj, mem.size()));
        }

        auto compressed = std::make_shared<buffer>();
        *compressed = message::compressPayload(Communicator::the().clusterManager().archiveCompressionMode(), *snd, mem,
                                               Communicator::the().clusterManager().archiveCompressionSpeed());

        snd->setDestId(req.senderId());
        snd->setDestRank(req.rank());
        send(*snd, compressed);
        //CERR << "sent " << snd->payloadSize() << "(" << snd->payloadRawSize() << ") bytes for " << req << " with " << *snd << std::endl;

        return true;
    });

    std::lock_guard<std::mutex> lock(m_sendTaskMutex);
    m_sendTasks.emplace_back(std::move(fut));

    return true;
}

bool DataManager::handlePriv(const message::SendObject &snd, buffer *payload)
{
#ifdef DEBUG
    if (snd.isArray()) {
        CERR << "received array " << snd.objectId() << " for " << snd.referrer() << ", size=" << snd.payloadSize()
             << std::endl;
    } else {
        CERR << "received object " << snd.objectId() << " for " << snd.referrer() << ", size=" << snd.payloadSize()
             << std::endl;
    }
#endif

    auto payload2 = std::make_shared<buffer>(std::move(*payload));
    auto fut = std::async(std::launch::async, [this, snd, payload2]() {
        buffer uncompressed = decompressPayload(snd, *payload2.get());
        vecistreambuf<buffer> membuf(uncompressed);

        if (snd.isArray()) {
            // an array was received
            vistle::iarchive memar(membuf);
            ArrayLoader loader(snd.objectId(), snd.objectType(), memar);
            if (!loader.load()) {
                CERR << "failed to restore array " << snd.objectId() << std::endl;
                return false;
            }

            std::unique_lock<std::mutex> lock(m_requestArrayMutex);
            //CERR << "restored array " << snd.objectId() << ", dangling in memory" << std::endl;
            auto it = m_requestedArrays.find(snd.objectId());
            if (it == m_requestedArrays.end()) {
                CERR << "restored array " << snd.objectId() << " for " << snd.referrer() << ", but did not find request"
                     << std::endl;
            }
            assert(it != m_requestedArrays.end());
            if (it != m_requestedArrays.end()) {
                auto handlers = std::move(it->second);
#ifdef DEBUG
                CERR << "restored array " << snd.objectId() << ", " << handlers.size() << " completion handler"
                     << std::endl;
#endif
                m_requestedArrays.erase(it);
                lock.unlock();
                for (const auto &completionHandler: handlers)
                    completionHandler(snd.objectId());
                lock.lock();
            }

            return true;
        }

        // an object was received
        std::string objName = snd.objectId();
        {
            std::lock_guard<std::mutex> lock(m_requestObjectMutex);
            auto objIt = m_requestedObjects.find(objName);
            if (objIt == m_requestedObjects.end()) {
                CERR << "object " << objName << " unexpected" << std::endl;
                return false;
            }
        }

        auto completionHandler = [this, objName]() mutable -> void {
            std::unique_lock<std::mutex> lock(m_requestObjectMutex);
            auto addIt = m_outstandingAdds.find(objName);
            if (addIt == m_outstandingAdds.end()) {
                // that's normal if a sub-object was loaded
                //CERR << "no outstanding add for " << objName << std::endl;
            } else {
                auto notify = std::move(addIt->second);
                m_outstandingAdds.erase(addIt);
                lock.unlock();
                for (const auto &add: notify) {
                    notifyTransferComplete(add);
                }
                lock.lock();
            }

            auto objIt = m_requestedObjects.find(objName);
            if (objIt != m_requestedObjects.end()) {
                auto handlers = std::move(objIt->second.completionHandlers);
                auto objref = objIt->second.obj;
                m_requestedObjects.erase(objIt);
                lock.unlock();
                auto obj = Shm::the().getObjectFromName(objName);
                if (!obj) {
                    if (auto incomplete = Shm::the().getObjectFromName(objName, false)) {
                        CERR << objName << " is INCOMPLETE" << std::endl;
                    } else {
                        CERR << "could not retrieve " << objName << " from shmem" << std::endl;
                    }
                }
                assert(obj);
                for (const auto &handler: handlers) {
                    handler(obj);
                }
                lock.lock();
            } else {
                CERR << "no outstanding object for " << objName << std::endl;
            }
        };

        vistle::iarchive memar(membuf);
        memar.setObjectCompletionHandler(completionHandler);
        std::shared_ptr<Fetcher> fetcher(new RemoteFetcher(this, snd.referrer(), snd.senderId(), snd.rank()));
        memar.setFetcher(fetcher);
        Object::const_ptr obj(Object::loadObject(memar));
        if (!obj) {
            CERR << "loading from archive failed for " << objName << std::endl;
        }
        assert(obj);

        std::lock_guard<std::mutex> lock(m_requestObjectMutex);
        auto objIt = m_requestedObjects.find(objName);
        if (objIt != m_requestedObjects.end()) {
            objIt->second.obj = obj;
        }

        return true;
    });

    std::lock_guard<std::mutex> lock(m_recvTaskMutex);
    m_recvTasks.emplace_back(std::move(fut));

    return true;
}

bool DataManager::handlePriv(const message::AddObjectCompleted &complete)
{
    return completeTransfer(complete);
}

void DataManager::recvLoop()
{
    for (;;) {
        bool gotMsg = false;
        if (m_dataSocket.is_open()) {
            message::Buffer buf;
            buffer payload;
            message::error_code ec;
            if (message::recv(m_dataSocket, buf, ec, false, &payload)) {
                gotMsg = true;
                std::lock_guard<std::mutex> guard(m_recvMutex);
                m_recvQueue.emplace_back(std::move(buf), std::move(payload));
                //CERR << "Data received" << std::endl;
            } else if (ec) {
                CERR << "Data communication error: " << ec.message() << std::endl;
            }

            std::lock_guard<std::mutex> guard(m_recvMutex);
            if (m_quit)
                break;
        }

        vistle::adaptive_wait(gotMsg, this);

        std::lock_guard<std::mutex> guard(m_recvMutex);
        if (m_quit)
            break;
    }
}

void DataManager::sendLoop()
{
    m_ioService.run();
}

void DataManager::cleanLoop()
{
    auto waitForTasks = [this](std::mutex &mutex, std::deque<std::future<bool>> &tasks, bool workDone,
                               const std::string &kind) -> bool {
        std::unique_lock<std::mutex> lock(mutex);
        while (tasks.size() > 100 || (!workDone && !tasks.empty())) {
            auto front = std::move(tasks.front());
            tasks.pop_front();
            lock.unlock();

            workDone = true;
            bool result = front.get();
            if (!result) {
                CERR << "asynchronous " << kind << " task failed" << std::endl;
            }
            lock.lock();
        }
        return workDone;
    };

    for (;;) {
        bool work = false;
        if (waitForTasks(m_sendTaskMutex, m_sendTasks, work, "send")) {
            work = true;
            std::lock_guard<std::mutex> guard(m_recvMutex);
            if (m_quit)
                break;
        }
        if (waitForTasks(m_recvTaskMutex, m_recvTasks, work, "receive")) {
            work = true;
            std::lock_guard<std::mutex> guard(m_recvMutex);
            if (m_quit)
                break;
        }

        vistle::adaptive_wait(work, this);
        std::lock_guard<std::mutex> guard(m_recvMutex);
        if (m_quit)
            break;
    }
}

DataManager::Msg::Msg(message::Buffer &&buf, buffer &&payload): buf(std::move(buf)), payload(std::move(payload))
{}

} // namespace vistle
