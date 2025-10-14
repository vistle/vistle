/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */


#include <boost/foreach.hpp>

#include <sys/types.h>

#include <cassert>
#include <cstdlib>
#include <future>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <queue>
#include <sstream>
#include <string>

#include <vistle/core/message.h>
#include <vistle/core/messagepayload.h>
#include <vistle/core/messagequeue.h>
#include <vistle/core/messagerouter.h>
#include <vistle/core/object.h>
#include <vistle/core/parameter.h>
#include <vistle/core/shm.h>
#include <vistle/util/directory.h>
#include <vistle/util/enum.h>
#include <vistle/util/stopwatch.h>
#include <vistle/util/sysdep.h>
#include <vistle/util/threadname.h>
#include <vistle/control/scanmodules.h>

#include "clustermanager.h"
#include "communicator.h"
#include "datamanager.h"

//#ifdef MODULE_THREAD
#include <vistle/module/module.h>
#include <vistle/util/filesystem.h>
#ifdef MODULE_STATIC
#include <vistle/module/moduleregistry.h>
#else
#include <boost/dll/import.hpp>
#endif
#include <boost/function.hpp>
//#endif

//#define QUEUE_DEBUG
#define BARRIER_DEBUG
//#define DEBUG


#define CERR std::cerr << "ClusterManager [" << m_rank << "/" << m_size << "] "

namespace bi = boost::interprocess;

namespace vistle {

using message::Id;

ClusterManager::Module::~Module()
{
    try {
        if (thread.joinable()) {
            thread.join();
        }
    } catch (std::exception &e) {
        std::cerr << "ClusterManager: ~Module: joining thread for module failed: " << e.what() << std::endl;
    }

    if (recvQueue) {
        recvQueue->signal();
    }
    try {
        if (messageThread.joinable()) {
            messageThread.join();
        } else {
            std::cerr << "ClusterManager: ~Module: messageThread for module not joinable" << std::endl;
        }
    } catch (std::exception &e) {
        std::cerr << "ClusterManager: ~Module: joining messageThread for module failed: " << e.what() << std::endl;
    }
}

void ClusterManager::Module::block(const message::Message &msg) const
{
#ifdef DEBUG
    std::cerr << "BLOCK: " << msg << std::endl;
#endif
    std::lock_guard<std::mutex> lock(messageMutex);
    blocked = true;
    blockers.emplace_back(msg);
}

void ClusterManager::Module::unblock(const message::Message &msg) const
{
    std::lock_guard<std::mutex> lock(messageMutex);
    assert(blocked);
    assert(!blockers.empty());

    if (!blocked)
        return;
    if (blockers.empty())
        return;

    auto isSame = [msg](const message::Buffer &buf) -> bool {
        return buf.uuid() == msg.uuid() && buf.type() == msg.type();
    };

    auto hasSame = [msg](const MessageWithPayload &mpl) -> bool {
        return mpl.buf.uuid() == msg.uuid() && mpl.buf.type() == msg.type();
    };

    if (isSame(blockers.front())) {
#ifdef DEBUG
        std::cerr << "UNBLOCK: found as frontmost of " << blockers.size() << " blockers: " << msg << std::endl;
#endif
        blockers.pop_front();
        message::Buffer buf(msg);
        assert(blockedMessages.front().buf.type() == msg.type());
        assert(blockedMessages.front().buf.uuid() == msg.uuid());
        blockedMessages.front().payload.ref();
        if (blockedMessages.front().payload)
            buf.setPayloadName(blockedMessages.front().payload.name());
        blockedMessages.pop_front();
        sendQueue->send(msg);
        if (blockers.empty()) {
#ifdef DEBUG
            std::cerr << "UNBLOCK: completely unblocked" << std::endl;
#endif
            blocked = false;
            while (!blockedMessages.empty()) {
                auto &mpl = blockedMessages.front();
                assert((mpl.buf.payloadSize() == 0 && !mpl.payload) || (mpl.buf.payloadSize() > 0 && mpl.payload));
                mpl.payload.ref();
                if (mpl.payload)
                    mpl.buf.setPayloadName(mpl.payload.name());
                sendQueue->send(mpl.buf);
                blockedMessages.pop_front();
            }
        } else {
            const auto &uuid = blockers.front().uuid();
            while (!blockedMessages.empty() && blockedMessages.front().buf.uuid() != uuid) {
                auto &mpl = blockedMessages.front();
                assert((mpl.buf.payloadSize() == 0 && !mpl.payload) || (mpl.buf.payloadSize() > 0 && mpl.payload));
                mpl.payload.ref();
                if (mpl.payload)
                    mpl.buf.setPayloadName(mpl.payload.name());
                sendQueue->send(mpl.buf);
                blockedMessages.pop_front();
            }
        }
    } else {
#ifdef DEBUG
        std::cerr << "UNBLOCK: " << blockers.size() << " blockers, frontmost: " << blockers.front() << ", received "
                  << msg << std::endl;
#endif
        auto it = std::find_if(blockers.begin(), blockers.end(), isSame);
        if (it == blockers.end()) {
            std::cerr << "UNBLOCK: " << msg << " not found in blockers:" << std::endl;
            for (const auto &b: blockers) {
                std::cerr << "   " << b << std::endl;
            }
            assert(it != blockers.end());
        } else {
            //std::cerr << "UNBLOCK: found in blockers" << std::endl;
            blockers.erase(it);
        }
        auto it2 = std::find_if(blockedMessages.begin(), blockedMessages.end(), hasSame);
        assert(it2 != blockedMessages.end());
        if (it2 != blockedMessages.end()) {
            //std::cerr << "UNBLOCK: updating message" << std::endl;
            it2->buf = msg;
            it2->buf.setPayloadName(it2->payload.name());
        }
    }
}

bool ClusterManager::Module::send(const message::Message &msg, const MessagePayload &payload) const
{
    message::Buffer buf(msg);
    if (msg.payloadSize() > 0 && payload) {
        buf.setPayloadName(payload.name());
    } else {
        buf.setPayloadName(std::string());
    }
    std::lock_guard<std::mutex> lock(messageMutex);
    if (blocked) {
        if (msg.payloadSize() > 0)
            blockedMessages.emplace_back(buf, payload);
        else
            blockedMessages.emplace_back(buf);
        return true;
    } else if (sendQueue) {
        if (msg.payloadSize() > 0 && payload)
            payload->ref();
        return sendQueue->send(buf);
    }
    return false;
}

bool ClusterManager::Module::update() const
{
    std::lock_guard<std::mutex> lock(messageMutex);
    if (sendQueue)
        return sendQueue->progress();
    return false;
}

void ClusterManager::Module::delay(const message::Message &msg, const MessagePayload &payload)
{
    delayedMessages.emplace_back(msg, payload);
}

bool ClusterManager::Module::processDelayed(bool *haveExecute)
{
    if (haveExecute)
        *haveExecute = false;
    while (haveDelayed()) {
        bool ret = true;
        if (Communicator::the().getRank() == 0) {
            if (ranksStarted == 0) {
                auto &mpl = delayedMessages.front();
                auto &msg = mpl.buf;
                auto type = msg.type();
                ret = Communicator::the().broadcastAndHandleMessage(mpl.buf, mpl.payload);
                delayedMessages.pop_front();
                if (type == message::EXECUTE) {
                    if (haveExecute)
                        *haveExecute = true;
                    break;
                }
            }
        }
        if (!ret)
            return false;
    }
    return true;
}

bool ClusterManager::Module::haveDelayed() const
{
    return !delayedMessages.empty();
}

ClusterManager::ClusterManager(boost::mpi::communicator comm, const std::vector<std::string> &hosts)
: m_comm(comm)
, m_portManager(new PortManager(this))
, m_stateTracker(message::Id::Invalid, std::string("ClusterManager state rk") + std::to_string(m_comm.rank()),
                 m_portManager)
, m_traceMessages(message::INVALID)
, m_quitFlag(false)
, m_rank(m_comm.rank())
, m_size(hosts.size())
, m_barrierActive(false)
{
    m_portManager->setTracker(&m_stateTracker);

    m_runningMap[Id::Vistle];
    m_runningMap[Id::Config];
}

ClusterManager::~ClusterManager()
{
    m_portManager->setTracker(nullptr);
}

void ClusterManager::init()
{
    m_stateTracker.setId(hubId());
}

const StateTracker &ClusterManager::state() const
{
    return m_stateTracker;
}

template<typename ValueType>
ValueType getSessionParameter(const StateTracker &state, const char *name)
{
    auto val = ValueType();
    auto param = std::dynamic_pointer_cast<ParameterBase<Integer>>(state.getParameter(Id::Vistle, name)).get();
    if (param)
        val = static_cast<ValueType>(param->getValue());
    return val;
}

template<>
Float getSessionParameter<Float>(const StateTracker &state, const char *name)
{
    auto val = Float();
    auto param = std::dynamic_pointer_cast<ParameterBase<Float>>(state.getParameter(Id::Vistle, name)).get();
    if (param)
        val = param->getValue();
    return val;
}

message::CompressionMode ClusterManager::archiveCompressionMode() const
{
    return getSessionParameter<message::CompressionMode>(state(), "archive_compression");
}

int ClusterManager::archiveCompressionSpeed() const
{
    return getSessionParameter<Integer>(state(), "archive_compression_speed");
}

const CompressionSettings &ClusterManager::compressionSettings()
{
    //TODO: find out why values don't change when setting them in the GUI's Session Parameter Menu?
    if (!m_compressionSettingsValid) {
        m_compressionSettingsValid = true;

        auto &cs = m_compressionSettings;
        cs.mode = getSessionParameter<FieldCompressionMode>(state(), CompressionSettings::p_mode);

        cs.zfpMode = getSessionParameter<FieldCompressionZfpMode>(state(), CompressionSettings::p_zfpMode);
        cs.zfpRate = getSessionParameter<Float>(state(), CompressionSettings::p_zfpRate);
        cs.zfpPrecision = getSessionParameter<Integer>(state(), CompressionSettings::p_zfpPrecision);
        cs.zfpAccuracy = getSessionParameter<Float>(state(), CompressionSettings::p_zfpAccuracy);

        cs.szAlgo = getSessionParameter<FieldCompressionSzAlgo>(state(), CompressionSettings::p_szAlgo);
        cs.szError = getSessionParameter<FieldCompressionSzError>(state(), CompressionSettings::p_szError);

        cs.szAbsError = getSessionParameter<Float>(state(), CompressionSettings::p_szAbsError);
        cs.szRelError = getSessionParameter<Float>(state(), CompressionSettings::p_szRelError);
        cs.szPsnrError = getSessionParameter<Float>(state(), CompressionSettings::p_szPsnrError);
        cs.szL2Error = getSessionParameter<Float>(state(), CompressionSettings::p_szL2Error);

        cs.bigWhoopNPar = getSessionParameter<Integer>(state(), CompressionSettings::p_bigWhoopNPar);
        cs.bigWhoopRate = std::to_string(getSessionParameter<Float>(state(), CompressionSettings::p_bigWhoopRate));
    }
    return m_compressionSettings;
}

int ClusterManager::getRank() const
{
    return m_rank;
}

int ClusterManager::getSize() const
{
    return m_size;
}

int ClusterManager::idToHub(int id) const
{
    return m_stateTracker.getHub(id);
}

int ClusterManager::hubId() const
{
    return Communicator::the().hubId();
}


bool ClusterManager::isLocal(int id) const
{
    if (id == Id::LocalHub)
        return true;
    if (Id::isHub(id)) {
        return (id == hubId());
    }
    int hub = idToHub(id);
    return hub == hubId();
}

std::string ClusterManager::getModuleName(int id) const
{
    return m_stateTracker.getModuleName(id);
}


bool ClusterManager::dispatch(bool &received)
{
    bool done = false;

#if 0
    if (m_modulePriorityChange != m_stateTracker.graphChangeCount()) {
        std::priority_queue<StateTracker::Module, std::vector<StateTracker::Module>, CompModuleHeight> pq;

        // handle messages from modules closer to sink first
        // - should allow for objects to travel through the pipeline more quickly
        m_modulePriorityChange = m_stateTracker.graphChangeCount();
        for (auto m: m_stateTracker.runningMap) {
            const auto &mod = m.second;
            pq.emplace(mod);
        }

        while (!pq.empty()) {
            m_modulePriority.emplace_back(pq.top());
            pq.pop();
        }
    }

    // handle messages from modules
    for (const auto &mod: m_modulePriority) {
        const int modId = mod.id;

        // keep messages from modules that have already reached a barrier on hold
        if (m_reachedSet.find(modId) != m_reachedSet.end())
            continue;

        if (mod.hub == hubId()) {
            bool recv = false;
            message::Buffer buf;
            std::shared_ptr<message::MessageQueue> mq;
            auto it = m_runningMap.find(modId);
            if (it != m_runningMap.end()) {
                it->second.update();
                mq = it->second.recvQueue;
            }
            if (mq) {
                try {
                    recv = mq->tryReceive(buf);
                } catch (boost::interprocess::interprocess_exception &ex) {
                    CERR << "receive mq " << ex.what() << std::endl;
                    exit(-1);
                }
            }

            if (recv) {
                received = true;
                MessagePayload pl;
                if (buf.payloadSize() > 0) {
                    pl = Shm::the().getArrayFromName<char>(buf.payloadName());
                }
                if (!Communicator::the().handleMessage(buf, pl))
                    done = true;
                pl.unref();
            }
        }
    }
#else
    std::deque<MessageWithPayload> delayed;
    for (auto &id_mod: m_runningMap) {
        auto &id = id_mod.first;
        auto &mod = id_mod.second;
        if (m_reachedSet.find(id) != m_reachedSet.end())
            continue;
        // process messages that have been delayed because of a previous barrier
        auto &incoming = mod.incomingMessages;
        while (!incoming.empty()) {
            if (!Communicator::the().handleMessage(incoming.front().buf, incoming.front().payload))
                done = true;
            incoming.pop_front();
        }
        mod.update();
    }

    std::deque<MessageWithPayload> incoming;
    {
        std::lock_guard<std::mutex> lock(m_incomingMutex);
        std::swap(m_incomingMessages, incoming);
    }
    while (!incoming.empty()) {
        int sender = incoming.front().buf.senderId();
        bool barrierReached = m_reachedSet.find(sender) != m_reachedSet.end();
        if (!barrierReached) {
            if (!Communicator::the().handleMessage(incoming.front().buf, incoming.front().payload))
                done = true;
        }
        auto it = m_runningMap.find(sender);
        if (barrierReached) {
            if (it != m_runningMap.end()) {
                it->second.incomingMessages.emplace_back(incoming.front().buf, incoming.front().payload);
            }
        } else {
            if (it != m_runningMap.end()) {
                it->second.update();
            }
        }
        incoming.pop_front();
    }
#endif

    for (auto &mod: m_runningMap) {
        bool barrierReached = m_reachedSet.find(mod.first) != m_reachedSet.end();
        if (!barrierReached)
            mod.second.update();
    }

    if (m_quitFlag) {
        if (numRunning() == 0)
            return false;

#if 0
      CERR << numRunning() << " modules still running..." << std::endl;
#endif
    }

    return !done;
}

bool ClusterManager::sendAll(const message::Message &message, const MessagePayload &payload) const
{
    // no module has id Invalid
    return sendAllOthers(message::Id::Invalid, message, payload);
}

bool ClusterManager::sendAllLocal(const message::Message &message, const MessagePayload &payload) const
{
    // no module has id Invalid
    return sendAllOthers(message::Id::Invalid, message, payload, true);
}

bool ClusterManager::sendAllOthers(int excluded, const message::Message &message, const MessagePayload &payload,
                                   bool localOnly) const
{
    message::Buffer buf(message);
    if (!localOnly) {
        buf.setDestId(Id::ForBroadcast);
        if (Communicator::the().isMaster()) {
            if (getRank() == 0)
                sendHub(buf, payload);
        } else {
            int senderHub = message.senderId();
            if (senderHub >= Id::ModuleBase)
                senderHub = idToHub(senderHub);
            if (senderHub == hubId()) {
                if (getRank() == 0)
                    sendHub(buf, payload);
            }
        }
        return true;
    }

    // handle messages to modules
    for (auto it = m_runningMap.begin(), next = it; it != m_runningMap.end(); it = next) {
        // modules might be removed during message processing
        next = it;
        ++next;

        const int modId = it->first;
        if (modId == excluded)
            continue;
        const auto &mod = it->second;
        const int hub = idToHub(modId);

        if (hub == hubId()) {
            mod.send(message, payload);
        }
    }

    return true;
}

bool ClusterManager::sendUi(const message::Message &message, const MessagePayload &payload) const
{
    return sendHub(message, payload);
}

bool ClusterManager::sendHub(const message::Message &message, const MessagePayload &payload, int destHub) const
{
    if (getRank() != 0 && !message::Router::the().toRank0(message)) {
        return true;
    }

    message::Buffer buf(message);
    if (Id::isHub(destHub))
        buf.setDestId(destHub);
    buf.setPayloadName(std::string());
    return Communicator::the().sendHub(buf, payload);
}

bool ClusterManager::sendMessage(const int moduleId, const message::Message &message, int destRank,
                                 const MessagePayload &payload) const
{
    const int hub = idToHub(moduleId);

    message::Buffer buf(message);
    if (payload)
        buf.setPayloadName(payload.name());
    if (Id::isModule(moduleId) && (hub == hubId() || hub == Id::LocalHub || hub == Id::LocalManager)) {
        //CERR << "local send to " << moduleId << ": " << buf << std::endl;
        if (destRank == -1 || destRank == getRank()) {
            RunningMap::const_iterator it = m_runningMap.find(moduleId);
            if (it == m_runningMap.end()) {
                CERR << "sendMessage: module " << moduleId << " not found" << std::endl;
                std::cerr << "  message: " << buf << std::endl;
                return true;
            }

            auto &mod = it->second;
            if (buf.type() == message::ADDOBJECT) {
                auto &addObj = buf.as<message::AddObject>();
                if (addObj.isUnblocking()) {
                    mod.unblock(addObj);
                    return true;
                }
                if (addObj.isBlocker()) {
                    mod.block(addObj);
                }
            }
            mod.send(buf, payload);
        } else {
            Communicator::the().sendMessage(moduleId, message, destRank, payload);
        }
    } else {
        CERR << "remote send to " << moduleId << ": " << message << std::endl;
        buf.setDestId(moduleId);
        buf.setDestRank(destRank);
        sendHub(buf, payload);
    }

    return true;
}

bool ClusterManager::handle(const message::Buffer &message, const MessagePayload &payload)
{
    using namespace vistle::message;

    if (message.destId() == Id::ForBroadcast) {
        return sendHub(message, payload);
    }

    if (message.type() == m_traceMessages || m_traceMessages == ANY) {
        CERR << "handle: " << message << std::endl;
    }

    switch (message.type()) {
    case CONNECT:
    case DISCONNECT:
    case SPAWN:
        // handled in handlePriv(...)
        break;
    case message::TRACE: {
        const Trace &trace = message.as<Trace>();
        handlePriv(trace);
        break;
    }
    default:
        if (payload)
            m_stateTracker.handle(message, payload->data(), payload->size());
        else
            m_stateTracker.handle(message, nullptr);
        break;
    }

    bool result = true;

    int senderHub = message.senderId();
    if (senderHub >= Id::ModuleBase)
        senderHub = idToHub(senderHub);
    int destHub = message.destId();
    if (destHub >= Id::ModuleBase)
        destHub = idToHub(destHub);
    if (message.typeFlags() & Broadcast || message.destId() == Id::Broadcast) {
#if 0
      if (message.senderId() != hubId() && senderHub == hubId()) {
         CERR << "BC: " << message << std::endl;
         if (getRank() == 0)
            sendHub(message, payload);
      }
#endif
        if (message.typeFlags() & BroadcastModule) {
            sendAllLocal(message, payload);
        }
    }
    if (message::Id::isModule(message.destId())) {
        if (destHub == hubId()) {
            //CERR << "module: " << message << std::endl;
            if (message.type() != message::EXECUTE && message.type() != message::CANCELEXECUTE &&
                message.type() != message::SETPARAMETER) {
                return sendMessage(message.destId(), message, -1, payload);
            }
        } else if (!message.wasBroadcast()) {
            return sendHub(message, payload);
        }
    }
    if (message::Id::isHub(message.destId()) || message.destId() == message::Id::Config ||
        message.destId() == message::Id::Vistle) {
        if (destHub != hubId() || message.type() == message::EXECUTE || message.type() == message::CANCELEXECUTE ||
            message.type() == message::COVER) {
            if (!message.wasBroadcast()) {
                return sendHub(message, payload);
            }
        }
    }

    switch (message.type()) {
    case message::IDENTIFY: {
        const auto &id = message.as<message::Identify>();
        CERR << "Identify message: " << id << std::endl;
        assert(id.identity() == message::Identify::REQUEST);
        if (getRank() == 0) {
            message::Identify ident(id, message::Identify::MANAGER);
            ident.setNumRanks(m_size);
            ident.setPid(getpid());
            ident.computeMac();
            sendHub(ident);
        }
        break;
    }

    case message::ADDHUB: {
        const auto &addhub = message.as<message::AddHub>();
        if (addhub.id() == hubId()) {
            scanModules(Communicator::the().m_vistleRoot, Communicator::the().m_buildType);
        }
        break;
    }

    case message::CREATEMODULECOMPOUND: {
        buffer pl(payload->begin(), payload->end());
        ModuleCompound comp(message.as<message::CreateModuleCompound>(), pl);
        AvailableModule::Key key(comp.hub(), comp.name());
        auto av = comp.transform();
        av.setHub(hubId());
        av.send(std::bind(&Communicator::sendHub, &Communicator::the(), std::placeholders::_1, std::placeholders::_2));
        m_localModules[key] = std::move(av);
        break;
    }

    case message::QUIT: {
        const message::Quit &quit = message.as<Quit>();
        result = handlePriv(quit);
        break;
    }

    case message::SPAWN: {
        const message::Spawn &spawn = message.as<Spawn>();
        result = handlePriv(spawn);
        break;
    }

    case message::CONNECT: {
        const message::Connect &connect = message.as<Connect>();
        result = handlePriv(connect);
        break;
    }

    case message::DISCONNECT: {
        const message::Disconnect &disc = message.as<Disconnect>();
        result = handlePriv(disc);
        break;
    }

    case message::MODULEEXIT: {
        const message::ModuleExit &moduleExit = message.as<ModuleExit>();
        result = handlePriv(moduleExit);
        break;
    }

    case message::EXECUTE: {
        const message::Execute &exec = message.as<Execute>();
        result = handlePriv(exec);
        break;
    }

    case message::CANCELEXECUTE: {
        const message::CancelExecute &cancel = message.as<CancelExecute>();
        result = handlePriv(cancel);
        break;
    }

    case message::ADDOBJECT: {
        const message::AddObject &m = message.as<AddObject>();
        result = handlePriv(m);
        break;
    }

    case message::ADDOBJECTCOMPLETED: {
        const message::AddObjectCompleted &m = message.as<AddObjectCompleted>();
        result = handlePriv(m);
        break;
    }

    case message::EXECUTIONPROGRESS: {
        const message::ExecutionProgress &prog = message.as<ExecutionProgress>();
        result = handlePriv(prog);
        break;
    }

    case message::BUSY: {
        const message::Busy &busy = message.as<Busy>();
        result = handlePriv(busy);
        break;
    }

    case message::IDLE: {
        const message::Idle &idle = message.as<Idle>();
        result = handlePriv(idle);
        break;
    }

    case message::SETPARAMETER: {
        const message::SetParameter &m = message.as<SetParameter>();
        result = handlePriv(m);
        break;
    }

    case message::SETPARAMETERCHOICES: {
        const message::SetParameterChoices &m = message.as<SetParameterChoices>();
        result = handlePriv(m, payload);
        break;
    }

    case message::BARRIER: {
        const message::Barrier &m = message.as<Barrier>();
        result = handlePriv(m);
        break;
    }

    case message::BARRIERREACHED: {
        const message::BarrierReached &m = message.as<BarrierReached>();
        result = handlePriv(m);
        break;
    }

    case message::SENDTEXT: {
        const message::SendText &m = message.as<SendText>();
        result = handlePriv(m, payload);
        break;
    }

    case message::ITEMINFO: {
        const message::ItemInfo &m = message.as<ItemInfo>();
        result = handlePriv(m, payload);
        break;
    }

    case message::REQUESTTUNNEL: {
        const message::RequestTunnel &m = message.as<RequestTunnel>();
        result = handlePriv(m);
        break;
    }

    case message::DATATRANSFERSTATE: {
        const message::DataTransferState &m = message.as<DataTransferState>();
        result = handlePriv(m);
        break;
    }

    case message::SETNAME: {
        const message::SetName &m = message.as<SetName>();
        result = handlePriv(m);
        break;
    }

    case message::REMOVEHUB:
    case message::STARTED:
    case message::ADDPORT:
    case message::ADDPARAMETER:
    case message::REMOVEPARAMETER:
    case message::MODULEAVAILABLE:
    case message::REPLAYFINISHED:
    case message::REDUCEPOLICY:
    case message::SCHEDULINGPOLICY:
    case message::OBJECTRECEIVEPOLICY:
    case message::UPDATESTATUS:
    case message::TRACE:
    case message::EXECUTIONDONE:
        break;

    default:

        CERR << "unhandled message from (id " << message.senderId() << " rank " << message.rank() << ") "
             << "type " << message.type() << std::endl;

        break;
    }

    if (result) {
        if (message.typeFlags() & TriggerQueue) {
            replayMessages();
        }
    } else {
        if (message.typeFlags() & QueueIfUnhandled) {
            queueMessage(message);
            result = true;
        }
    }

    return result;
}

bool ClusterManager::handlePriv(const message::Trace &trace)
{
    CERR << "handle: " << trace << std::endl;

    if (Id::isModule(trace.module())) {
        sendMessage(trace.module(), trace);
    } else if (trace.module() == Id::Broadcast) {
        sendAllLocal(trace);
    }

    if (trace.module() == hubId() || !(Id::isModule(trace.module() || Id::isHub(trace.module())))) {
        if (trace.on())
            m_traceMessages = trace.messageType();
        else
            m_traceMessages = message::INVALID;
    }

    Communicator::the().dataManager().trace(m_traceMessages);

    return true;
}

bool ClusterManager::handlePriv(const message::SetName &setname)
{
    return sendAllLocal(setname);
}

bool ClusterManager::handlePriv(const message::Quit &quit)
{
    if (quit.id() == Id::Broadcast || quit.id() == hubId()) {
        sendAllLocal(quit);
        this->quit();
    }

    return true;
}

bool ClusterManager::handlePriv(const message::Spawn &spawn)
{
    CERR << "handling spawn: " << spawn << std::endl;

    if (spawn.spawnId() == Id::Invalid) {
        // ignore messages where master hub did not yet create an id
        return true;
    }

    if (spawn.destId() == Id::Broadcast || spawn.destId() == Id::NextHop) {
        m_stateTracker.handle(spawn, nullptr);
        sendAllLocal(spawn);
        return true;
    }

    if (spawn.destId() != hubId()) {
        return true;
    }
    int newId = spawn.spawnId();
    std::string name(spawn.getName());

    Module &mod = m_runningMap[newId];
    std::string smqName = message::MessageQueue::createName("send", newId, m_rank);
    std::string rmqName = message::MessageQueue::createName("recv", newId, m_rank);

    try {
        mod.sendQueue.reset(message::MessageQueue::create(smqName));
        mod.recvQueue.reset(message::MessageQueue::create(rmqName));
    } catch (bi::interprocess_exception &ex) {
        mod.sendQueue.reset();
        mod.recvQueue.reset();
        CERR << "spawn mq " << ex.what() << std::endl;
        exit(-1);
    }

    mod.sendQueue->makeNonBlocking();

    std::thread mt([this, newId, name, &mod]() {
        std::string tname = std::to_string(newId) + "mq:" + name;
        setThreadName(tname);

        for (;;) {
            message::Buffer buf;
            try {
                if (!mod.recvQueue->receive(buf))
                    return;
            } catch (boost::interprocess::interprocess_exception &ex) {
                CERR << "receive mq " << ex.what() << std::endl;
                return;
            }

            MessagePayload pl;
            if (buf.payloadSize() > 0) {
                pl = Shm::the().getArrayFromName<char>(buf.payloadName());
            }
            std::lock_guard<std::mutex> guard(m_incomingMutex);
            m_incomingMessages.emplace_back(buf, pl);

            if (buf.type() == message::MODULEEXIT)
                return;
        }
    });
    mod.messageThread = std::move(mt);

    m_comm.barrier();

    message::SpawnPrepared prep(spawn);

    auto handleFail = [&]() {
        // synthesize ModuleExit for module that has failed to start
        message::ModuleExit m(true);
        m.setSenderId(newId);
        m_stateTracker.handle(m, nullptr);
        if (getRank() == 0)
            return sendHub(m);
        return true;
    };

    std::string pluginpath;
    bool loadModulePlugin = spawn.asPlugin();
    if (loadModulePlugin) {
        Directory dir(Communicator::the().m_vistleRoot, Communicator::the().m_buildType);
        pluginpath = dir.moduleplugin() + "/lib" + name + ".so";
    }
#ifdef MODULE_THREAD
    loadModulePlugin = true;
#endif

    if (loadModulePlugin) {
        AvailableModule::Key key(hubId(), name);
        //AvailableModule::Key key(0, name);
        const auto &avail = m_localModules;
        auto it = avail.find(key);
        if (it == avail.end()) {
            CERR << "did not find module " << name << std::endl;
            return handleFail();
        } else {
            auto &m = it->second;
            if (pluginpath.empty())
                pluginpath = m.path();
            try {
#ifdef MODULE_STATIC
                mod.newModule = ModuleRegistry::the().moduleFactory(m.path());
#else
                mod.newModule = boost::dll::import_alias<Module::NewModuleFunc>(pluginpath, "newModule",
                                                                                boost::dll::load_mode::default_mode);
#endif
            } catch (const std::exception &e) {
                CERR << "importing module " << name << "(" << pluginpath << ") failed: " << e.what() << std::endl;
                std::vector<const char *> vars;
#if defined(_WIN32)
                vars.push_back("PATH");
#elif defined(__APPLE__)
                vars.push_back("DYLD_LIBRARY_PATH");
                vars.push_back("DYLD_FRAMEWORK_PATH");
                vars.push_back("DYLD_FALLBACK_LIBRARY_PATH");
                vars.push_back("DYLD_FALLBACK_FRAMEWORK_PATH");
#else
                vars.push_back("LD_LIBRARY_PATH");
                vars.push_back("LD_PRELOAD");
#endif
                for (auto v: vars) {
                    const char *val = getenv(v);
                    if (val) {
                        CERR << "  " << v << ": " << val << std::endl;
                    } else {
                        CERR << "  " << v << ": not set" << std::endl;
                    }
                }
            }
            if (mod.newModule) {
                boost::mpi::communicator ncomm(m_comm, boost::mpi::comm_duplicate);
                std::thread t([newId, name, ncomm, &mod]() {
                    std::string tname = std::to_string(newId) + "mn:" + name;
                    setThreadName(tname);
                    //CERR << "thread for module " << name << ":" << newId << std::endl;
                    mod.instance = mod.newModule(name, newId, ncomm);
                    if (mod.instance)
                        mod.instance->eventLoop();

                    mod.instance.reset();
                });
                mod.thread = std::move(t);
                prep.setAsPlugin(true);
            } else {
                CERR << "no newModule method for module " << name << std::endl;
                auto it = m_runningMap.find(newId);
                if (it != m_runningMap.end()) {
                    m_runningMap.erase(it);
                }
                return handleFail();
            }
        }
    } else {
        prep.setDestId(Id::LocalHub);
        if (getRank() == 0)
            sendHub(prep);
    }
    prep.setDestId(Id::MasterHub);
    prep.setNotify(true);
    if (getRank() == 0)
        sendHub(prep);

    // inform newly started module about current parameter values of other modules
    auto state = m_stateTracker.getLockedState();
    for (const auto &m: state.messages) {
        MessagePayload pl;
        message::Buffer buf(m.message);
        if (m.payload) {
            pl.construct(m.payload->size());
            std::copy(m.payload->begin(), m.payload->end(), pl->begin());
            buf.setPayloadName(pl.name());
        }
        sendMessage(newId, buf, -1, pl);
    }

    return true;
}

bool ClusterManager::handlePriv(const message::Connect &connect)
{
    if (connect.isNotification()) {
        CERR << "new connection: " << connect << std::endl;
        Communicator::the().comm().barrier();
        m_stateTracker.handle(connect, nullptr);
        int modFrom = connect.getModuleA();
        int modTo = connect.getModuleB();
        if (isLocal(modFrom))
            sendMessage(modFrom, connect);
        if (isLocal(modTo))
            sendMessage(modTo, connect);
        if (isLocal(modFrom)) {
            const char *portFrom = connect.getPortAName();
            const char *portTo = connect.getPortBName();
            unsigned numAvailable = 0;
            const unsigned Error(~0u);
            auto destPort = portManager().findPort(modTo, portTo);
            if (!destPort)
                numAvailable = Error;
            auto it = m_stateTracker.runningMap.find(modTo);
            if (it == m_stateTracker.runningMap.end()) {
                numAvailable = Error;
            }
            std::vector<Object::const_ptr> objs;
            if (const Port *from = portManager().findPort(modFrom, portFrom)) {
                PortKey key(from);
                auto it = m_outputObjects.find(key);
                if (numAvailable >= 0 && it != m_outputObjects.end()) {
                    objs.reserve(it->second.objects.size());
                    for (auto &name: it->second.objects) {
                        auto obj = Shm::the().getObjectFromName(name);
                        if (!obj) {
                            CERR << "did not find " << name << std::endl;
                            numAvailable = Error;
                            break;
                        }
                        objs.emplace_back(obj);
                        ++numAvailable;
                    }
                }
                //CERR << "local conn: on this rank all available=" << numAvailable << std::endl;
            } else {
                CERR << "local conn: did not find port" << std::endl;
                numAvailable = Error;
            }
            numAvailable =
                boost::mpi::all_reduce(Communicator::the().comm(), numAvailable, boost::mpi::maximum<unsigned>());
            if (numAvailable != Error && numAvailable > 0) {
                //CERR << "re-sending " << objs.size() << " objects" << std::endl;
                for (auto obj: objs) {
                    message::AddObject add(portFrom, obj, portTo);
                    add.setSenderId(modFrom);
                    add.setDestId(modTo);
                    add.setRank(m_rank);
                    handlePriv(add);
                }
            }
        }
    } else {
        sendHub(connect);
    }
    return true;
}

bool ClusterManager::handlePriv(const message::Disconnect &disconnect)
{
    if (disconnect.isNotification()) {
        m_stateTracker.handle(disconnect, nullptr);
        int modFrom = disconnect.getModuleA();
        int modTo = disconnect.getModuleB();
        if (isLocal(modFrom))
            sendMessage(modFrom, disconnect);
        if (isLocal(modTo))
            sendMessage(modTo, disconnect);
    } else if (isLocal(disconnect.senderId())) {
        sendHub(disconnect);
    }

    return true;
}

bool ClusterManager::handlePriv(const message::ModuleExit &moduleExit)
{
    const int mod = moduleExit.senderId();
    const bool crashed = moduleExit.isCrashed();

    //CERR << " Module [" << mod << "] quit" << std::endl;

    if (moduleExit.isForwarded()) {
        sendAllOthers(mod, moduleExit, MessagePayload(), true);

        RunningMap::iterator it = m_runningMap.find(mod);
        if (it != m_runningMap.end()) {
            if (crashed) {
                (void)m_crashedMap[mod];
            }
            m_runningMap.erase(it);
        } else if (!crashed) {
            it = m_crashedMap.find(mod);
            if (it != m_crashedMap.end()) {
                m_crashedMap.erase(it);
            }
        }
        return true;
    }

    const bool local = isLocal(mod);
    if (local) {
        if (m_runningMap.find(mod) == m_runningMap.end() && m_crashedMap.find(mod) == m_crashedMap.end()) {
            CERR << " Module [" << mod << "] quit, but not found in running map" << std::endl;
            return true;
        }

        auto ports = portManager().getOutputPorts(mod);
        for (const auto *port: ports) {
            m_outputObjects.erase(port);
        }

        ModuleSet::iterator it = m_reachedSet.find(mod);
        if (it != m_reachedSet.end()) {
            m_reachedSet.erase(it);
        }
    }

    if (m_rank == 0) {
        message::ModuleExit exit = moduleExit;
        exit.setForwarded();
        sendHub(exit);
        if (!Communicator::the().broadcastAndHandleMessage(exit))
            return false;
    }

    return true;
}

bool ClusterManager::handlePriv(const message::Execute &exec)
{
    assert(exec.getModule() >= Id::ModuleBase);
    RunningMap::iterator i = m_runningMap.find(exec.getModule());
    if (i == m_runningMap.end()) {
        if (isLocal(exec.getModule())) {
            CERR << "did not find module to be executed: " << exec.getModule() << std::endl;
        }
        return true;
    }

    auto &mod = i->second;
    switch (exec.what()) {
    case message::Execute::Upstream: {
        CERR << "sending upstream exec to " << exec.getModule() << std::endl;
        mod.send(exec);
        break;
    }
    case message::Execute::Prepare: {
        CERR << "sending prepare to " << exec.getModule() << ", checking for execution" << std::endl;
        assert(!mod.prepared);
        assert(mod.reduced);
        mod.prepared = true;
        mod.reduced = false;
        mod.send(exec);
        checkExecuteObject(exec.getModule());
        break;
    }
    case message::Execute::Reduce: {
        CERR << "sending reduce to " << exec.getModule() << std::endl;
        assert(mod.prepared);
        assert(!mod.reduced);
        mod.prepared = false;
        mod.reduced = true;
        mod.send(exec);
        break;
    }
    case message::Execute::ComputeExecute: {
        if (exec.wasBroadcast()) {
            mod.send(exec);
            mod.prepared = false;
            mod.reduced = true;
        } else if (Communicator::the().getRank() == 0) {
            CERR << "non-broadcast Execute: " << exec << std::endl;
            if (mod.ranksStarted > 0) {
                mod.delay(exec);
            } else {
                assert(!mod.prepared);
                mod.prepared = false;
                mod.reduced = true;
                Communicator::the().broadcastAndHandleMessage(exec);
            }
        }
        break;
    }
    case message::Execute::ComputeObject: {
        assert(mod.prepared);
        //CERR << exec << std::endl;
        auto it = m_stateTracker.runningMap.find(exec.getModule());
        auto pol = message::SchedulingPolicy::Single;
        if (it != m_stateTracker.runningMap.end()) {
            pol = it->second.schedulingPolicy;
        }
        if (exec.wasBroadcast() || pol == message::SchedulingPolicy::Single) {
            mod.reduced = false;
            if (exec.wasBroadcast()) {
                CERR << "executing after broadcast: " << exec << std::endl;
            }
            mod.send(exec);
        } else {
            if (m_rank == 0) {
                bool doExec = pol == message::SchedulingPolicy::Gang;
                int numObjects = 0;
                if (pol == message::SchedulingPolicy::LazyGang) {
                    if (ssize_t(mod.objectCount.size()) < getSize())
                        mod.objectCount.resize(getSize());
                    ++mod.objectCount[exec.rank()];
                    numObjects = std::accumulate(mod.objectCount.begin(), mod.objectCount.end(), 0);
                    if (numObjects > 0 && numObjects >= getSize() * .2) {
                        doExec = true;
                        for (auto &c: mod.objectCount) {
                            if (c > 0) {
                                --c;
                            }
                        }
                    }
                }
                if (doExec) {
                    CERR << "having " << numObjects << ", executing " << exec.getModule() << std::endl;
                    Communicator::the().broadcastAndHandleMessage(exec);
                }
            }
        }
        break;
    }
    }

    return true;
}

bool ClusterManager::handlePriv(const message::CancelExecute &cancel)
{
    assert(cancel.getModule() >= Id::ModuleBase);
    RunningMap::iterator i = m_runningMap.find(cancel.getModule());
    if (i == m_runningMap.end()) {
        CERR << "did not find module to cancel execution: " << cancel.getModule() << std::endl;
        return true;
    }

    auto &mod = i->second;
    if (cancel.wasBroadcast()) {
        mod.send(cancel);
        return true;
    }
    if (m_rank > 0) {
        return Communicator::the().forwardToMaster(cancel);
    }

    CERR << "non-broadcast CancelExecute: " << cancel << std::endl;
    return Communicator::the().broadcastAndHandleMessage(cancel);
}

bool ClusterManager::addObjectSource(const message::AddObject &addObj)
{
    const Port *port = portManager().findPort(addObj.senderId(), addObj.getSenderPort());
    if (!port) {
        CERR << "AddObject [" << addObj.objectName() << "] to port [" << addObj.getSenderPort() << "] of ["
             << addObj.senderId() << "]: port not found" << std::endl;
        //assert(port);
        return true;
    }

    const bool resendAfterConnect = message::Id::isModule(addObj.destId());

    if (!resendAfterConnect) {
        PortKey key(port);
        auto gen = addObj.meta().generation();
        auto iter = addObj.meta().iteration();
        auto &cache = m_outputObjects[key];
        if (cache.generation != gen || cache.iteration != iter) {
#ifdef DEBUG
            CERR << "clearing cache for " << addObj.senderId() << ":" << addObj.getSenderPort() << std::endl;
#endif
            cache.objects.clear();
        }
        cache.objects.emplace_back(addObj.objectName());
        cache.generation = gen;
        cache.iteration = iter;
#ifdef DEBUG
        CERR << "caching " << addObj.objectName() << " for " << addObj.senderId() << ":" << addObj.getSenderPort()
             << ", port=" << port << std::endl;
#endif
    }

    const Port::ConstPortSet *list = portManager().getConnectionList(port);
    if (!list) {
        //CERR << "AddObject [" << addObj.objectName() << "] to port [" << addObj.getSenderPort() << "] of [" << addObj.senderId() << "]: connection list not found" << std::endl;
        assert(list);
        return true;
    }
    Port::ConstPortSet ports;
    if (resendAfterConnect) {
        ports.emplace(portManager().findPort(addObj.destId(), addObj.getDestPort()));
        list = &ports;
    }

    // if object was generated locally, forward message to remote hubs with connected modules
    std::set<int> receivingHubs; // make sure that message is only sent once per remote hub
    for (const Port *destPort: *list) {
        int destId = destPort->getModuleID();

        if (!isLocal(destId)) {
            const int hub = idToHub(destId);
            if (receivingHubs.find(hub) == receivingHubs.end()) {
                receivingHubs.insert(hub);
                message::AddObject a(addObj);
                a.setDestId(hub);
                a.setDestRank(0);
                Communicator::the().dataManager().prepareTransfer(a);
                // TODO: serialize object into message payload - it's small and saves a round trip
                sendHub(a, MessagePayload(), hub);
            }
        }
    }

    return true;
}

bool ClusterManager::addObjectDestination(const message::AddObject &addObj, Object::const_ptr obj)
{
    const Port *port = portManager().findPort(addObj.senderId(), addObj.getSenderPort());
    if (!port) {
        CERR << "AddObject [" << addObj.objectName() << "] to port [" << addObj.getSenderPort() << "] of ["
             << addObj.senderId() << "]: port not found" << std::endl;
        //assert(port);
        return true;
    }
    const Port::ConstPortSet *list = portManager().getConnectionList(port);
    if (!list) {
        //CERR << "AddObject [" << addObj.objectName() << "] to port [" << addObj.getSenderPort() << "] of [" << addObj.senderId() << "]: connection list not found" << std::endl;
        assert(list);
        return true;
    }

    const bool resendAfterConnect = message::Id::isModule(addObj.destId());
    Port::ConstPortSet ports;
    if (resendAfterConnect) {
        ports.emplace(portManager().findPort(addObj.destId(), addObj.getDestPort()));
        list = &ports;
    }

    for (const Port *destPort: *list) {
        int destId = destPort->getModuleID();
        if (!isLocal(destId))
            continue;

        auto it = m_stateTracker.runningMap.find(destId);
        if (it == m_stateTracker.runningMap.end()) {
            if (m_stateTracker.quitMap.find(destId) != m_stateTracker.quitMap.end()) {
                CERR << "port connection to module " << destId << ":" << destPort->getName() << ", which has crashed"
                     << std::endl;
                continue;
            }
            CERR << "port connection to module " << destId << ":" << destPort->getName() << ", which is not running"
                 << std::endl;
            assert("port connection to module that is not running" == 0);
            continue;
        }
        auto &destMod = it->second;

        message::AddObject addObj2(addObj);
        addObj2.setRank(m_rank); // object is/will be present on this rank
        addObj2.setDestId(destId);
        addObj2.setDestPort(destPort->getName());
        addObj2.setDestRank(-1);
        if (obj) {
            addObj2.setObject(obj);
        } else {
            // receiving module will wait until it is unblocked
            addObj2.setBlocker();
        }

        bool broadcast = false;
        if (destMod.objectPolicy == message::ObjectReceivePolicy::Local) {
            CERR << "LOCAL object add at " << destId << ": " << addObj2.objectName() << std::endl;
            if (!sendMessage(destId, addObj2))
                return false;
            portManager().addObject(destPort);

            if (!checkExecuteObject(destId))
                return false;
        } else {
            CERR << "BROADCAST object add at " << destId << ": " << addObj2.objectName() << std::endl;
            broadcast = true;
            if (!Communicator::the().broadcastAndHandleMessage(addObj2))
                return false;
        }

        if (!obj) {
            // block messages of receiving module until remote object is available
            assert(!isLocal(addObj.senderId()));
            auto it = m_runningMap.find(destId);
            if (it != m_runningMap.end()) {
                Communicator::the().dataManager().requestObject(
                    addObj, addObj.objectName(), [this, addObj, addObj2, broadcast](Object::const_ptr newobj) mutable {
                        auto obj = addObj.getObject();
                        assert(obj);
                        assert(obj->getName() == newobj->getName());
                        addObj2.setObject(newobj);
                        obj.reset();
                        // unblock receiving module
                        addObj2.setUnblocking();

                        if (broadcast) {
                            Communicator::the().broadcastAndHandleMessage(addObj2);
                        } else {
                            sendMessage(addObj2.destId(), addObj2);
                        }
                    });
            }
        }
    }

    return true;
}

bool ClusterManager::handlePriv(const message::AddObject &addObj)
{
    const bool resendAfterConnect = message::Id::isModule(addObj.destId());

    if (addObj.wasBroadcast()) {
        assert(!resendAfterConnect);
        assert(isLocal(addObj.destId()));
        if (!sendMessage(addObj.destId(), addObj))
            return false;

        if (addObj.isUnblocking()) {
            return true;
        }

        if (auto destPort = portManager().getPort(addObj.destId(), addObj.getDestPort()))
            portManager().addObject(destPort);
        else
            return false;

        if (!checkExecuteObject(addObj.destId()))
            return false;

        return true;
    }

    const bool localAdd = isLocal(addObj.senderId());
    if (resendAfterConnect) {
        assert(localAdd);
    }

    if (localAdd) {
        addObjectSource(addObj);
    }

    //CERR << "ADDOBJECT: " << addObj << ", local=" << localAdd << std::endl;
    Object::const_ptr obj;

    bool onThisRank = false;
    int destRank = -1;
    if (localAdd) {
        if (addObj.rank() == getRank()) {
            onThisRank = true;
            obj = addObj.takeObject();
        } else {
            obj = Shm::the().getObjectFromName(addObj.objectName());
        }
        //CERR << "ADDOBJECT: local, name=" << obj->getName() << ", refcount=" << obj->refcount() << std::endl;
    } else {
        int block = addObj.meta().block();
        if (block >= 0) {
            destRank = block % getSize();
        }
        onThisRank = destRank == getRank() || (getRank() == 0 && destRank == -1);

        //CERR << "ADDOBJECT from remote, handling on rank " << destRank << std::endl;
        if (onThisRank) {
            obj = addObj.getObject();
            if (obj)
                Communicator::the().dataManager().notifyTransferComplete(addObj);
        } else {
            return sendMessage(hubId(), addObj, destRank);
        }
    }

    assert(onThisRank);

    if (localAdd && onThisRank) {
        assert(obj);
    }
    assert(!obj || obj->refcount() >= 1);

    return addObjectDestination(addObj, obj);
}

bool ClusterManager::checkExecuteObject(int destId)
{
    if (!isReadyForExecute(destId))
        return true;

    int numconn = 0;
    for (const auto input: portManager().getConnectedInputPorts(destId)) {
        if (input->flags() & Port::NOCOMPUTE)
            continue;
        ++numconn;
        if (!portManager().hasObject(input)) {
            return true;
        }
    }
    CERR << "checkExecuteObject " << destId << ": " << numconn << " connections" << std::endl;
    if (numconn == 0)
        return true;
    for (const auto input: portManager().getConnectedInputPorts(destId)) {
        if (input->flags() & Port::NOCOMPUTE)
            continue;
        portManager().popObject(input);
    }

    auto it = m_stateTracker.runningMap.find(destId);
    if (it == m_stateTracker.runningMap.end()) {
        CERR << "port connection to module that is not running" << std::endl;
        assert("port connection to module that is not running" == 0);
        return true;
    }
    auto &destMod = it->second;
    message::Execute c(message::Execute::ComputeObject, destId);
    c.setDestId(destId);
    //c.setUuid(addObj.uuid());
    if (destMod.schedulingPolicy == message::SchedulingPolicy::Single) {
        sendMessage(destId, c);
    } else if (destMod.schedulingPolicy == message::SchedulingPolicy::Gang) {
        c.setAllRanks(true);
        CERR << "checkExecuteObject " << destId << ": exec b/c gang scheduling: " << c << std::endl;
        if (!Communicator::the().broadcastAndHandleMessage(c))
            return false;
    } else if (destMod.schedulingPolicy == message::SchedulingPolicy::LazyGang) {
        if (getRank() == 0) {
            handle(c);
        } else {
            if (!Communicator::the().forwardToMaster(c))
                return false;
        }
    }

    return checkExecuteObject(destId);
}

bool ClusterManager::handlePriv(const message::AddObjectCompleted &complete)
{
    return Communicator::the().dataManager().completeTransfer(complete);
}

bool ClusterManager::handlePriv(const message::ExecutionProgress &prog)
{
    const bool localSender = idToHub(prog.senderId()) == hubId();
    RunningMap::iterator i = m_runningMap.find(prog.senderId());
    ClusterManager::Module *mod = nullptr;
    if (i == m_runningMap.end()) {
        assert(localSender == false);
    } else {
        mod = &i->second;
    }

    auto i2 = m_stateTracker.runningMap.find(prog.senderId());
    if (i2 == m_stateTracker.runningMap.end()) {
        CERR << "handle ExecutionProgress: module " << prog.senderId() << " not found, msg=" << prog << std::endl;
        return false;
    }
    auto &modState = i2->second;

    // initiate reduction if requested by module

    bool handleOnMaster = false;
    // forward message to remote hubs...
    std::set<int> receivingHubs; // ...but make sure that message is only sent once per remote hub
    if (localSender) {
        for (auto output: portManager().getConnectedOutputPorts(prog.senderId())) {
            const Port::ConstPortSet *list = portManager().getConnectionList(output);
            for (const Port *destPort: *list) {
                int destId = destPort->getModuleID();
                if (!isLocal(destId)) {
                    int hub = idToHub(destId);
                    receivingHubs.insert(hub);
                }
                const auto it = m_stateTracker.runningMap.find(destId);
                if (it == m_stateTracker.runningMap.end()) {
                    CERR << "did not find " << destId << " in runningMap, but it is connected to " << prog.senderId()
                         << ":" << output->getName() << std::endl;
                    abort();
                }
                assert(it != m_stateTracker.runningMap.end());
                const auto &destState = it->second;
                if (destState.reducePolicy != message::ReducePolicy::Never) {
                    bool gang = destState.schedulingPolicy == message::SchedulingPolicy::Gang ||
                                destState.schedulingPolicy == message::SchedulingPolicy::LazyGang;
                    if (gang || destState.reducePolicy != message::ReducePolicy::Locally)
                        handleOnMaster = true;
                }
            }
        }
    }
    if (!receivingHubs.empty())
        handleOnMaster = true;

    bool gang = modState.schedulingPolicy == message::SchedulingPolicy::Gang ||
                modState.schedulingPolicy == message::SchedulingPolicy::LazyGang;
    bool localReduce = (modState.reducePolicy == message::ReducePolicy::Locally && !gang) ||
                       modState.reducePolicy == message::ReducePolicy::Never;
    if (!localReduce)
        handleOnMaster = true;
    // for reliable tracking that a module has finished we have to forward all messages to rank 0
    handleOnMaster = true;
    if (localSender && handleOnMaster && m_rank != 0) {
        //CERR << "exec progr: forward to master" << std::endl;
        return Communicator::the().forwardToMaster(prog);
    }

    bool readyForPrepare = false, readyForReduce = false;
    bool execDone = false;
    bool unqueueExecute = false;
    if (localSender) {
        CERR << "ExecutionProgress " << prog.stage() << " received from " << prog.senderId() << ":" << prog.rank()
             << ", before: #started=" << mod->ranksStarted << ", #fin=" << mod->ranksFinished << std::endl;
    } else {
        CERR << "ExecutionProgress " << prog.stage() << " received from " << prog.senderId() << ":" << prog.rank()
             << std::endl;
    }
    switch (prog.stage()) {
    case message::ExecutionProgress::Start: {
        readyForPrepare = true;
        if (localSender) {
            assert(mod->ranksFinished < m_size);
            ++mod->ranksStarted;
            if (handleOnMaster)
                readyForPrepare = mod->ranksStarted == m_size;
        }
        break;
    }

    case message::ExecutionProgress::Finish: {
        readyForReduce = true;
        if (localSender) {
            ++mod->ranksFinished;
            if (mod->ranksFinished == m_size) {
                if (mod->ranksStarted != m_size) {
                    CERR << "ExecutionProgress::Finish: mismatch for module " << prog.senderId()
                         << ": m_size=" << m_size << ", started=" << mod->ranksStarted << std::endl;
                }
                assert(mod->ranksStarted >= m_size);
                mod->ranksStarted -= m_size;
                mod->ranksFinished = 0;
                execDone = true;
                if (handleOnMaster)
                    unqueueExecute = true;
            } else {
                if (handleOnMaster)
                    readyForReduce = false;
            }
        }
        break;
    }
    }

    if (localSender) {
        CERR << "ExecutionProgress " << prog.stage() << " received from " << prog.senderId() << ":" << prog.rank()
             << ", after: #started=" << mod->ranksStarted << ", #fin=" << mod->ranksFinished << std::endl;
    }

    if (readyForPrepare || readyForReduce) {
        assert(!(readyForPrepare && readyForReduce));
        for (auto hub: receivingHubs) {
            message::Buffer buf(prog);
            buf.setForBroadcast(false);
            buf.setDestRank(0);
            sendMessage(hub, buf);
        }
    }

    //CERR << prog.senderId() << " ready for prepare: " << readyForPrepare << ", reduce: " << readyForReduce << std::endl;
    for (auto output: portManager().getConnectedOutputPorts(prog.senderId())) {
        const Port::ConstPortSet *list = portManager().getConnectionList(output);
        for (const Port *destPort: *list) {
            if (!(destPort->flags() & Port::NOCOMPUTE)) {
                if (readyForPrepare)
                    portManager().resetInput(destPort);
                if (readyForReduce)
                    portManager().finishInput(destPort);
            }
        }
    }

    for (auto output: portManager().getConnectedOutputPorts(prog.senderId())) {
        const Port::ConstPortSet *list = portManager().getConnectionList(output);
        for (const Port *destPort: *list) {
            if (!(destPort->flags() & Port::NOCOMPUTE)) {
                bool allReadyForPrepare = true, allReadyForReduce = true;
                const int destId = destPort->getModuleID();
                auto allInputs = portManager().getConnectedInputPorts(destId);
                for (auto input: allInputs) {
                    if (input->flags() & Port::NOCOMPUTE)
                        continue;
                    if (!portManager().isReset(input))
                        allReadyForPrepare = false;
                    if (!portManager().isFinished(input))
                        allReadyForReduce = false;
                }
                //CERR << "exec prog: checking module " << destId << ":" << destPort->getName() << std::endl;
                auto it = m_stateTracker.runningMap.find(destId);
                assert(it != m_stateTracker.runningMap.end());
                if (it->second.reducePolicy != message::ReducePolicy::Never ||
                    it->second.schedulingPolicy == message::SchedulingPolicy::Gang ||
                    it->second.schedulingPolicy == message::SchedulingPolicy::LazyGang) {
                    bool broadcast = handleOnMaster || it->second.reducePolicy != message::ReducePolicy::Locally;
                    if (allReadyForPrepare) {
                        for (auto input: allInputs) {
                            portManager().popReset(input);
                        }
                        //CERR << "Exec prepare 1" << std::endl;
                        if (isLocal(destId)) {
                            //CERR << "Exec prepare 2" << std::endl;
                            auto exec = message::Execute(message::Execute::Prepare, destId);
                            exec.setDestId(destId);
                            if (broadcast) {
                                //CERR << "Exec prepare 3" << std::endl;
                                if (m_rank == 0)
                                    if (!Communicator::the().broadcastAndHandleMessage(exec))
                                        return false;
                            } else {
                                //CERR << "Exec prepare 4" << std::endl;
                                handlePriv(exec);
                            }
                        }
                    }
                    if (allReadyForReduce) {
                        if (handleOnMaster && localSender) {
                            assert(mod->ranksFinished == 0);
                        }
                        // process all objects which are still in the queue before calling reduce()
                        if (isLocal(destId)) {
                            if (it->second.schedulingPolicy == message::SchedulingPolicy::LazyGang) {
                                broadcast = true;
                                RunningMap::iterator i = m_runningMap.find(destId);
                                assert(i != m_runningMap.end());
                                auto &destMod = i->second;

                                assert(destMod.prepared);
                                assert(!destMod.reduced);
                                if (m_rank == 0) {
                                    if (ssize_t(destMod.objectCount.size()) < getSize())
                                        destMod.objectCount.resize(getSize());
                                    int maxNumObject = 0;
                                    for (size_t r = 0; r < destMod.objectCount.size(); ++r) {
                                        auto &c = destMod.objectCount[r];
                                        if (c > 0) {
                                            CERR << "flushing " << c << " objects for rank " << r << ", module "
                                                 << destId << std::endl;
                                        }
                                        if (c > maxNumObject)
                                            maxNumObject = c;
                                        c = 0;
                                    }
                                    for (int i = 0; i < maxNumObject; ++i) {
                                        message::Execute exec(message::Execute::ComputeObject, destId);
                                        if (!Communicator::the().broadcastAndHandleMessage(exec))
                                            return false;
                                    }
                                    assert(std::accumulate(destMod.objectCount.begin(), destMod.objectCount.end(), 0) ==
                                           0);
                                }
                            }
                        }
                        for (auto input: allInputs) {
                            portManager().popFinish(input);
                        }
                        if (isLocal(destId)) {
                            auto exec = message::Execute(message::Execute::Reduce, destId);
                            exec.setDestId(destId);
                            if (broadcast) {
                                if (m_rank == 0)
                                    if (!Communicator::the().broadcastAndHandleMessage(exec))
                                        return false;
                            } else {
                                handlePriv(exec);
                            }
                        }
                    }
                }
            }
        }
    }

    if (unqueueExecute) {
        bool haveExecute = false;
        mod->processDelayed(&haveExecute);
        if (haveExecute) {
            execDone = false;
        }
    }

    if (execDone) {
        if (m_rank == 0) {
            auto done = message::ExecutionDone();
            done.setSenderId(prog.senderId());
            sendHub(done, MessagePayload(), message::Id::MasterHub);
        }
    }

    return true;
}

bool ClusterManager::handlePriv(const message::Busy &busy)
{
    if (getRank() == 0) {
        int id = busy.senderId();
        auto &mod = m_runningMap[id];
        if (mod.busyCount == 0) {
            message::Buffer buf(busy);
            buf.setDestId(Id::UI);
            sendHub(buf, MessagePayload(), Id::MasterHub);
        }
        ++mod.busyCount;
    } else {
        Communicator::the().forwardToMaster(busy);
    }
    return true;
}

bool ClusterManager::handlePriv(const message::Idle &idle)
{
    if (getRank() == 0) {
        int id = idle.senderId();
        auto &mod = m_runningMap[id];
        --mod.busyCount;
        if (mod.busyCount == 0) {
            message::Buffer buf(idle);
            buf.setDestId(Id::UI);
            sendHub(buf, MessagePayload(), Id::MasterHub);
        }
    } else {
        Communicator::the().forwardToMaster(idle);
    }
    return true;
}

bool ClusterManager::handlePriv(const message::SetParameter &setParam)
{
#ifdef DEBUG
    CERR << "SetParameter: " << setParam << std::endl;
#endif

    assert(setParam.getModule() >= Id::ModuleBase || setParam.getModule() == Id::Vistle ||
           setParam.getModule() == Id::Config || Id::isHub(setParam.getModule()));
    if (setParam.getModule() == Id::Vistle)
        m_compressionSettingsValid = false;
    int sender = setParam.senderId();
    int dest = setParam.destId();
    RunningMap::iterator i = m_runningMap.find(setParam.getModule());
    Module *mod = nullptr;
    if (i == m_runningMap.end()) {
        if (isLocal(setParam.getModule()) && setParam.getModule() != message::Id::Config) {
            CERR << "did not find module for SetParameter: " << setParam.getModule() << ": " << setParam << std::endl;
        }
    } else {
        mod = &i->second;
    }

    bool handled = true;
    if (message::Id::isModule(dest)) {
        // message to owning module
        if (setParam.wasBroadcast()) {
            if (mod) {
                mod->send(setParam);
            }
        } else {
            return sendHub(setParam, MessagePayload(), dest);
        }
    } else if (message::Id::isModule(sender) &&
               (sender == setParam.getModule() || setParam.getModule() == message::Id::Config)) {
        // message from owning module
        auto param = getParameter(sender, setParam.getName());
        if (param) {
            setParam.apply(param);
        }
        if (dest == Id::ForBroadcast || dest == Id::Config) {
            sendHub(setParam, MessagePayload(), dest);
            return true;
        } else if (!Communicator::the().isMaster()) {
            sendAllOthers(sender, setParam, MessagePayload(), true);
        }
    }

    if (!mod)
        return true;

    return handled;
}

bool ClusterManager::handlePriv(const message::SetParameterChoices &setChoices, const MessagePayload &payload)
{
#ifdef DEBUG
    CERR << "SetParameterChoices: " << setChoices << std::endl;
#endif

    assert(payload);
    if (!payload) {
        return false;
    }

    bool handled = true;
    int sender = setChoices.senderId();
    int dest = setChoices.destId();
    buffer data(payload->begin(), payload->end());
    auto pl = message::getPayload<message::SetParameterChoices::Payload>(data);
    if (message::Id::isModule(dest)) {
        // message to owning module
        auto param = getParameter(dest, setChoices.getName());
        if (param) {
            setChoices.apply(param, pl);
        }
    } else if (message::Id::isModule(sender)) {
        // message from owning module
        auto param = getParameter(sender, setChoices.getName());
        if (param) {
            setChoices.apply(param, pl);
        }
        if (dest == Id::ForBroadcast) {
            sendHub(setChoices, payload, Id::MasterHub);
        } else if (!Communicator::the().isMaster()) {
            sendAllOthers(sender, setChoices, payload, true);
        }
    }

    return handled;
}

bool ClusterManager::handlePriv(const message::Barrier &barrier)
{
    m_barrierActive = true;
    //sendHub(barrier);
    CERR << "Barrier [" << barrier.uuid() << ": " << barrier.info() << "]" << std::endl;
    m_barrierUuid = barrier.uuid();
    return sendAllLocal(barrier);
}

bool ClusterManager::handlePriv(const message::BarrierReached &barrReached)
{
#ifdef BARRIER_DEBUG
    auto info = m_stateTracker.barrierInfo(barrReached.uuid());
    CERR << "BarrierReached [barrier " << barrReached.uuid() << ", module " << barrReached.senderId() << ":, " << info
         << "]" << std::endl;
#endif
    assert(m_barrierActive);
    if (barrReached.uuid() != m_barrierUuid) {
        CERR << "BARRIER: BarrierReached message " << barrReached << " ("
             << m_stateTracker.barrierInfo(barrReached.uuid()) << ") with wrong uuid, expected " << m_barrierUuid
             << " (" << m_stateTracker.barrierInfo(m_barrierUuid) << ")" << std::endl;
        return true;
    }

    if (Id::isModule(barrReached.senderId())) {
        assert(isLocal(barrReached.senderId()));
        m_reachedSet.insert(barrReached.senderId());
        if (getRank() == 0) {
            message::BarrierReached m(barrReached.uuid());
            m.setSenderId(barrReached.senderId());
            m.setDestId(Id::MasterHub);
            sendMessage(Id::MasterHub, m);
        }
    } else if (barrReached.senderId() == Id::MasterHub) {
        m_reachedSet.clear();
        m_barrierActive = false;
    } else {
        CERR << "BARRIER: BarrierReached message from invalid sender " << barrReached.senderId() << std::endl;
    }

    return true;
}

bool ClusterManager::handlePriv(const message::SendText &text, const MessagePayload &payload)
{
    if (Communicator::the().isMaster()) {
        message::Buffer buf(text);
        buf.setDestId(Id::MasterHub);
        sendHub(buf, payload);
    } else {
        sendHub(text, payload);
    }
    return true;
}

bool ClusterManager::handlePriv(const message::ItemInfo &info, const MessagePayload &payload)
{
    if (Communicator::the().isMaster()) {
        message::Buffer buf(info);
        buf.setDestId(Id::MasterHub);
        sendHub(buf, payload);
    } else {
        sendHub(info, payload);
    }
    return true;
}

bool ClusterManager::handlePriv(const message::RequestTunnel &tunnel)
{
    using message::RequestTunnel;

    CERR << "RequestTunnel: remove=" << tunnel.remove() << " ";
    std::cerr << "host=" << tunnel.destHost() << " ";
    if (tunnel.destType() == RequestTunnel::IPv6) {
        std::cerr << "addr=" << tunnel.destAddrV6();
    } else if (tunnel.destType() == RequestTunnel::IPv4) {
        std::cerr << "addr=" << tunnel.destAddrV4();
    } else if (tunnel.destType() == RequestTunnel::Hostname) {
        std::cerr << "addr=" << tunnel.destHost();
    }
    std::cerr << std::endl;

    if (m_rank > 0) {
        return Communicator::the().forwardToMaster(tunnel);
    }

    message::RequestTunnel tun(tunnel);
    tun.setDestId(Id::LocalHub);
    if (!tunnel.remove() && tunnel.destType() == RequestTunnel::Unspecified) {
        CERR << "RequestTunnel: fill in local address" << std::endl;
        try {
            auto addr = Communicator::the().m_hubSocket.local_endpoint().address();
            if (addr.is_v6()) {
                tun.setDestAddr(addr.to_v6());
            } else if (addr.is_v4()) {
                tun.setDestAddr(addr.to_v4());
            }
            CERR << "RequestTunnel: " << tun << std::endl;
        } catch (std::bad_cast &except) {
            CERR << "RequestTunnel: failed to convert local address to v6" << std::endl;
        }
    }

    return sendHub(tun);
}

bool ClusterManager::handlePriv(const message::DataTransferState &state)
{
    assert(m_rank == 0);
    if (ssize_t(m_numTransfering.size()) < m_size)
        m_numTransfering.resize(m_size);

    int r = state.rank();
    assert(r >= 0);
    m_totalNumTransferring -= m_numTransfering[r];
    m_numTransfering[r] = state.numTransferring();
    m_totalNumTransferring += m_numTransfering[r];

    std::stringstream str;
    if (m_totalNumTransferring == 0) {
        Communicator::the().clearStatus();
    } else {
        str << m_totalNumTransferring << " objects to transfer" << std::endl;
        auto now = Clock::time();
        if (now - m_lastStatusUpdateTime > 1.) {
            m_lastStatusUpdateTime = now;
            Communicator::the().setStatus(str.str(), message::UpdateStatus::Low);
        }
    }

    return true;
}

bool ClusterManager::quit()
{
    if (!m_quitFlag)
        sendAllLocal(message::Kill(message::Id::Broadcast));

    // receive all ModuleExit messages from modules
    // retry for some time, modules that don't answer might have crashed
    CERR << "waiting for " << numRunning() << " modules to quit" << std::endl;

    if (m_size > 1)
        m_comm.barrier();

    m_quitFlag = true;

    return numRunning() == 0;
}

bool ClusterManager::quitOk() const
{
    return m_quitFlag && numRunning() == 0;
}

PortManager &ClusterManager::portManager() const
{
    return *m_portManager;
}

std::vector<std::string> ClusterManager::getParameters(int id) const
{
    return m_stateTracker.getParameters(id);
}

std::shared_ptr<Parameter> ClusterManager::getParameter(int id, const std::string &name) const
{
    return m_stateTracker.getParameter(id, name);
}

void ClusterManager::queueMessage(const message::Message &msg)
{
    m_messageQueue.emplace_back(msg);
#ifdef QUEUE_DEBUG
    CERR << "queueing " << msg.type() << ", now " << m_messageQueue.size() << " in queue" << std::endl;
#endif
}

void ClusterManager::replayMessages()
{
    std::vector<message::Buffer> queue;
    std::swap(m_messageQueue, queue);
#ifdef QUEUE_DEBUG
    if (!queue.empty())
        CERR << "replaying " << queue.size() << " messages" << std::endl;
#endif
    for (const auto &m: queue) {
        Communicator::the().handleMessage(m);
    }
}

int ClusterManager::numRunning() const
{
    int n = 0;
    for (auto &m: m_runningMap) {
        int state = m_stateTracker.getModuleState(m.first);
        if ((state & StateObserver::Initialized) &&
            /* !(state & StateObserver::Killed) && */ !(state & StateObserver::Quit))
            ++n;
    }
    return n;
}

bool ClusterManager::isReadyForExecute(int modId) const
{
    //CERR << "checking whether " << modId << " can be executed: ";

    auto i = m_runningMap.find(modId);
    if (i == m_runningMap.end()) {
        //CERR << "module " << modId << " not found" << std::endl;
        return false;
    }
    auto &mod = i->second;

    auto i2 = m_stateTracker.runningMap.find(modId);
    if (i2 == m_stateTracker.runningMap.end()) {
        //CERR << "module " << modId << " not found by state tracker" << std::endl;
        return false;
    }
    auto &modState = i2->second;

    if (modState.reducePolicy == message::ReducePolicy::Never) {
        //CERR << "reduce policy Never" << std::endl;
        return true;
    }

    if (!mod.reduced && mod.prepared) {
        assert(mod.ranksFinished <= m_size);
        //CERR << "prepared & not reduced" << std::endl;
        return true;
    }

    //CERR << "prepared: " << mod.prepared << ", reduced: " << mod.reduced << std::endl;
    return false;
}

bool ClusterManager::scanModules(const std::string &prefix, const std::string &buildtype)
{
    bool result = true;
#if defined(MODULE_THREAD) && defined(MODULE_STATIC)
    result = ModuleRegistry::the().availableModules(m_localModules, hubId());
#else
#if !defined(MODULE_THREAD)
    if (getRank() == 0) {
#endif
        result = vistle::scanModules(prefix, buildtype, hubId(), m_localModules);
#if !defined(MODULE_THREAD)
    }
#endif
#endif

#if !defined(MODULE_THREAD)
    if (getRank() == 0) {
#endif
        // module aliases
        config::File modules("modules");
        for (auto &alias: modules.entries("alias")) {
            auto e = modules.value<std::string>("alias", alias)->value();
            AvailableModule::Key key(hubId(), e);
            auto it = m_localModules.find(key);
            if (it == m_localModules.end()) {
                CERR << "alias " << alias << " -> " << e << " not found" << std::endl;
                continue;
            }

            AvailableModule::Key keya(hubId(), alias);
            m_localModules[keya] =
                AvailableModule{hubId(), alias, it->second.path(), it->second.category(), it->second.description()};
        }
#if !defined(MODULE_THREAD)
    }
#endif

    if (getRank() == 0) {
        for (auto &p: m_localModules) {
            p.second.send(
                std::bind(&Communicator::sendHub, &Communicator::the(), std::placeholders::_1, std::placeholders::_2));
        }
    }

    return result;
}

} // namespace vistle
