/*
 * Vistle
 *
 * This once was started as Visualization Testing Laboratory for Exascale Computing (VISTLE) by Florian Niebling
 */
#include <boost/asio.hpp>
#include <boost/mpi.hpp>

#include <mpi.h>

#include <cstdlib>
#include <iostream>

#include <vistle/core/message.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/core/parameter.h>
#include <cassert>
#include <vistle/core/shm.h>
#include <vistle/core/messagepayload.h>
#include <vistle/util/sleep.h>
#include <vistle/util/tools.h>
#include <vistle/util/hostname.h>
#include <vistle/util/crypto.h>

#include "communicator.h"
#include "clustermanager.h"
#include "datamanager.h"

#if defined(MODULE_THREAD) && defined(MODULE_STATIC)
#include <vistle/module/moduleregistry.h>
#endif

#define CERR std::cerr << "comm [" << m_rank << "/" << m_size << "] "

namespace asio = boost::asio;
namespace mpi = boost::mpi;

namespace vistle {

using message::Id;

Communicator *Communicator::s_singleton = NULL;

Communicator::Communicator(int r, const std::vector<std::string> &hosts, boost::mpi::communicator comm)
: m_comm(comm)
, m_clusterManager(new ClusterManager(m_comm, hosts))
, m_dataManager(new DataManager(m_comm))
, m_hubId(message::Id::Invalid)
, m_rank(r)
, m_size(hosts.size())
, m_recvSize(0)
, m_hubSocket(m_ioContext)
{
    crypto::initialize();

    assert(s_singleton == NULL);
    s_singleton = this;

    message::DefaultSender::init(m_hubId, m_rank);

    // post requests for length of next MPI message
    if (m_size > 1) {
        MPI_Irecv(&m_recvSize, 1, MPI_UNSIGNED, MPI_ANY_SOURCE, TagStartBroadcast, comm, &m_reqAny);

        MPI_Irecv(m_recvBufToRank.data(), m_recvBufToRank.bufferSize(), MPI_BYTE, MPI_ANY_SOURCE, TagToRank, comm,
                  &m_reqToRank);
    }
}

Communicator &Communicator::the()
{
    assert(s_singleton && "make sure to use the vistle Python module only from within vistle");
    if (!s_singleton)
        exit(1);
    return *s_singleton;
}

void Communicator::setVistleRoot(const std::string &dir, const std::string &buildtype)
{
    m_vistleRoot = dir;
    m_buildType = buildtype;
}

int Communicator::hubId() const
{
    return m_hubId;
}

bool Communicator::isMaster() const
{
    assert(m_hubId <= Id::MasterHub); // make sure that hub id has already been set
    return m_hubId == Id::MasterHub;
}

void Communicator::setStatus(const std::string &text, int prio)
{
    message::UpdateStatus t(text, (message::UpdateStatus::Importance)prio);
    sendMessage(message::Id::ForBroadcast, t);
}

void Communicator::clearStatus()
{
    setStatus(std::string(), message::UpdateStatus::Bulk);
}

int Communicator::getRank() const
{
    return m_rank;
}

int Communicator::getSize() const
{
    return m_size;
}

bool Communicator::connectHub(std::string host, unsigned short port, unsigned short dataPort)
{
    if (dataPort == 0)
        dataPort = port;
    if (host == hostname())
        host = "localhost";
    if (getRank() == 0) {
        CERR << "connecting to hub on " << host << ":" << port << "..." << std::flush;
    }

    asio::ip::tcp::resolver resolver(m_ioContext);
    boost::system::error_code ec;
    auto ep = resolver.resolve(host, std::to_string(port), asio::ip::tcp::resolver::numeric_service, ec);
    if (ec) {
        CERR << "could not resolve host " << host << ":" << port << ": " << ec.message() << std::endl;
        return false;
    }
    m_dataEndpoint = resolver.resolve(host, std::to_string(dataPort), asio::ip::tcp::resolver::numeric_service, ec);
    if (ec) {
        CERR << "could not resolve host " << host << ":" << dataPort << ": " << ec.message() << std::endl;
        return false;
    }

    int ret = 1;
    if (getRank() == 0) {
        asio::connect(m_hubSocket, ep, ec);
        if (ec) {
            std::cerr << std::endl;
            CERR << "could not establish connection to hub at " << host << ":" << port << std::endl;
            ret = false;
        } else {
            std::cerr << " ok." << std::endl;
        }
    }

    MPI_Bcast(&ret, 1, MPI_INT, 0, m_comm);

    return ret;
}

mpi::communicator Communicator::comm() const
{
    return m_comm;
}

void Communicator::lock()
{
    m_mutex.lock();
}

void Communicator::unlock()
{
    m_mutex.unlock();
}

bool Communicator::connectData()
{
    return m_dataManager->connect(m_dataEndpoint);
}

bool Communicator::sendHub(const message::Message &message, const MessagePayload &payload)
{
    if (message.payloadSize() > 0) {
        assert(payload);
    }
    if (payload) {
        assert(payload->size() == message.payloadSize());
    }

    if (getRank() == 0) {
        message::error_code ec;
        if (payload) {
            if (message::send(m_hubSocket, message, ec, payload->data(), payload->size()))
                return true;
        } else {
            if (message::send(m_hubSocket, message, ec))
                return true;
        }
        if (ec) {
            CERR << "sending " << message << " to hub failed: " << ec.message() << std::endl;
        }
        return false;
    }
    return forwardToMaster(message, payload);
}

bool Communicator::run()
{
    bool work = false;
    while (dispatch(&work) && !m_terminate) {
        if (parentProcessDied())
            throw(except::parent_died());

        vistle::adaptive_wait(work, this);
    }
    CERR << "Comm: run done" << std::endl;
    return true;
}

bool Communicator::dispatch(bool *work)
{
    bool done = false;

    bool received = false;

    std::unique_lock guard(m_mutex);

    // check for new UIs and other network clients
    // handle or broadcast messages received from slaves (rank > 0)
    if (m_size > 1) {
        int flag;
        MPI_Status status;
        MPI_Test(&m_reqToRank, &flag, &status);
        if (flag && status.MPI_TAG == TagToRank) {
            received = true;
            message::Message *message = &m_recvBufToRank;
            MessagePayload payload;
            if (message->payloadSize() > 0) {
                payload.construct(message->payloadSize());
                MPI_Status status2;
                MPI_Recv(payload->data(), payload->size(), MPI_BYTE, status.MPI_SOURCE, TagToRank, m_comm, &status2);
                message->setPayloadName(payload.name());
            }
            if (m_rank == 0 && message->isForBroadcast()) {
                if (!broadcastAndHandleMessage(*message, payload)) {
                    CERR << "Quit reason: broadcast & handle" << std::endl;
                    done = true;
                }
            } else {
                if (!handleMessage(*message, payload)) {
                    CERR << "Quit reason: handle" << std::endl;
                    done = true;
                }
            }
            MPI_Irecv(m_recvBufToRank.data(), m_recvBufToRank.bufferSize(), MPI_BYTE, MPI_ANY_SOURCE, TagToRank, m_comm,
                      &m_reqToRank);
        }

        // test for message size from another MPI node
        //    - receive actual message from broadcast (on any rank)
        //    - receive actual message from slave rank (on rank 0) for broadcasting
        //    - handle message
        //    - post another MPI receive for size of next message
        MPI_Test(&m_reqAny, &flag, &status);
        if (flag && status.MPI_TAG == TagStartBroadcast) {
            if (m_recvSize > m_recvBufToAny.bufferSize()) {
                CERR << "invalid m_recvSize: " << m_recvSize << ", flag=" << flag
                     << ", status.MPI_SOURCE=" << status.MPI_SOURCE << std::endl;
            }
            assert(m_recvSize <= m_recvBufToAny.bufferSize());
            MPI_Bcast(m_recvBufToAny.data(), m_recvSize, MPI_BYTE, status.MPI_SOURCE, m_comm);

            unsigned recvSize = m_recvSize;

            // post new receive
            MPI_Irecv(&m_recvSize, 1, MPI_UNSIGNED, MPI_ANY_SOURCE, TagStartBroadcast, m_comm, &m_reqAny);

            if (recvSize > 0) {
                received = true;

                message::Message *message = &m_recvBufToAny;
#if 0
            printf("[%02d] message from [%02d] message type %d m_size %d\n",
                  m_rank, status.MPI_SOURCE, message->getType(), mpiMessageSize);
#endif
                MessagePayload payload;
                if (message->payloadSize() > 0) {
                    payload.construct(message->payloadSize());
                    MPI_Bcast(payload->data(), payload->size(), MPI_BYTE, status.MPI_SOURCE, m_comm);
                    message->setPayloadName(payload.name());
                }
                //CERR << "handle broadcast: " << *message << std::endl;
                if (!handleMessage(*message, payload)) {
                    CERR << "Quit reason: handle message received via broadcast: " << *message << std::endl;
                    done = true;
                }
            }
        }

#if 0
      if (m_ongoingSends.size() > 0) {
          CERR << m_ongoingSends.size() << " ongoing MPI sends" << std::endl;
      }
#endif
        for (auto it = m_ongoingSends.begin(), next = it; it != m_ongoingSends.end(); it = next) {
            if ((*it)->testComplete()) {
                next = m_ongoingSends.erase(it);
            } else {
                ++next;
            }
        }
    }

    guard.unlock();
    m_ioContext.poll();
    //guard.lock();

    if (m_rank == 0) {
        message::Buffer buf;
        message::error_code ec;
        buffer payload;
        if (!message::recv(m_hubSocket, buf, ec, false, &payload)) {
            if (ec) {
                CERR << "Quit reason: hub comm interrupted: " << ec.message() << std::endl;
                if (hubId() == Id::MasterHub)
                    broadcastAndHandleMessage(message::Quit());
                else
                    broadcastAndHandleMessage(message::Quit(hubId()));
                done = true;
            }
        } else {
            received = true;
            MessagePayload pl(payload);
            if (buf.destRank() == 0) {
                handleMessage(buf, pl);
            } else if (buf.destRank() >= 0) {
                startSend(buf.destRank(), buf, pl);
            } else if (!broadcastAndHandleMessage(buf, pl)) {
                CERR << "Quit reason: broadcast & handle 2: " << buf << buf << std::endl;
                done = true;
            }
        }
    }

    //guard.unlock();
    if (m_dataManager->dispatch())
        received = true;
    guard.lock();

    // test for messages from modules
    if (done) {
        CERR << "telling clusterManager to quit" << std::endl;
        if (m_clusterManager) {
            m_clusterManager->quit();
            done = false;
        }
    }
    if (m_clusterManager->quitOk()) {
        CERR << "Quit reason: clustermgr ready for quit" << std::endl;
        done = true;
    }
    if (!done && m_clusterManager) {
        done = !m_clusterManager->dispatch(received);
        if (done) {
            CERR << "Quit reason: clustermgr dispatch" << std::endl;
        }
    }
#if 0
   if (done) {
      delete m_clusterManager;
      m_clusterManager = nullptr;
   }
#endif

    if (work)
        *work = received;

    if (m_rank == 0 && done) {
        if (hubId() == Id::MasterHub)
            sendHub(message::Quit());
        else
            sendHub(message::Quit(hubId()));
    }

    return !done;
}

void Communicator::terminate()
{
    m_terminate = true;
}

bool Communicator::startSend(int destRank, const message::Message &message, const MessagePayload &payload)
{
    std::lock_guard guard(m_mutex);
    auto p = m_ongoingSends.emplace(new SendRequest(message));
    auto it = p.first;
    auto &sr = **it;
    MPI_Isend(sr.buf.data(), sr.buf.size(), MPI_BYTE, destRank, TagToRank, m_comm, &sr.req);
    if (sr.buf.payloadSize() > 0) {
        sr.payload = payload;
        MPI_Isend(sr.payload->data(), sr.payload->size(), MPI_BYTE, 0, TagToRank, m_comm, &sr.payload_req);
    }
    return true;
}

bool Communicator::SendRequest::waitComplete()
{
    MPI_Status status;
    MPI_Wait(&req, &status);
    if (buf.payloadSize() > 0)
        MPI_Wait(&payload_req, &status);
    return true;
}

bool Communicator::SendRequest::testComplete()
{
    int flag = 0;
    MPI_Status status;
    MPI_Test(&req, &flag, &status);
    if (!flag)
        return false;
    if (buf.payloadSize() == 0)
        return true;
    MPI_Test(&payload_req, &flag, &status);
    return flag;
}

bool Communicator::sendMessage(const int moduleId, const message::Message &message, int destRank,
                               const MessagePayload &payload)
{
    if (m_rank == destRank || destRank == -1) {
        return clusterManager().sendMessage(moduleId, message);
    }

    return startSend(destRank, message, payload);
}

bool Communicator::forwardToMaster(const message::Message &message, const MessagePayload &payload)
{
    if (message.payloadSize() > 0) {
        assert(payload);
    }
    if (payload) {
        assert(payload->size() == message.payloadSize());
    }

    assert(m_rank != 0);
    if (m_rank != 0) {
        return startSend(0, message, payload);
    }

    return true;
}

bool Communicator::broadcastAndHandleMessage(const message::Message &message, const MessagePayload &payload)
{
    assert(message.destRank() == -1);
    if (message.payloadSize() > 0) {
        assert(payload);
    }
    if (payload) {
        assert(payload->size() == message.payloadSize());
    }

    // message will be handled when received again from rank 0
    message::Buffer buf(message);
    if (m_rank > 0) {
        buf.setForBroadcast(true);
        buf.setWasBroadcast(false);
        return forwardToMaster(buf, payload);
    }

    buf.setForBroadcast(false);
    buf.setWasBroadcast(true);

    MessagePayload pl = payload;
    if (m_size > 0) {
        std::lock_guard guard(m_mutex);
        std::vector<MPI_Request> s(m_size);
        unsigned int size = buf.size();
        for (int index = 0; index < m_size; ++index) {
            if (index != m_rank) {
                MPI_Isend(&size, 1, MPI_UNSIGNED, index, TagStartBroadcast, m_comm, &s[index]);
            }
        }

        // wait for completion
        for (int index = 0; index < m_size; ++index) {
            if (index == m_rank)
                continue;

            MPI_Wait(&s[index], MPI_STATUS_IGNORE);
        }

        MPI_Bcast(buf.data(), buf.size(), MPI_BYTE, m_rank, m_comm);
        if (buf.payloadSize() > 0) {
            MPI_Bcast(pl->data(), pl->size(), MPI_BYTE, m_rank, m_comm);
        }
    }

    if (pl)
        buf.setPayloadName(pl.name());
    return handleMessage(buf, pl);
}

bool Communicator::handleMessage(const message::Buffer &message, const MessagePayload &payload)
{
    if (message.payloadSize() > 0) {
        assert(payload);
        assert(payload->size() == message.payloadSize());
    }

    std::lock_guard guard(m_mutex);

    if (message.type() == message::SETID) {
        const auto &set = message.as<message::SetId>();
        m_hubId = set.getId();
        CERR << "got id " << m_hubId << std::endl;
        message::DefaultSender::init(m_hubId, m_rank);
        Shm::the().setId(m_hubId);
        m_clusterManager->init();
        return connectData();
    }

    return m_clusterManager->handle(message, payload);
}

Communicator::~Communicator()
{
    for (auto it = m_ongoingSends.begin(), next = it; it != m_ongoingSends.end(); it = next) {
        (*it)->waitComplete();
        next = m_ongoingSends.erase(it);
    }

    delete m_dataManager;
    m_dataManager = nullptr;

    CERR << "shut down: deleting clusterManager" << std::endl;
    delete m_clusterManager;
    m_clusterManager = nullptr;

    CERR << "shut down: start init barrier" << std::endl;
    MPI_Barrier(m_comm);
    CERR << "shut down: done init BARRIER" << std::endl;

    if (m_size > 1) {
        MPI_Cancel(&m_reqAny);
        MPI_Wait(&m_reqAny, MPI_STATUS_IGNORE);
        MPI_Cancel(&m_reqToRank);
        MPI_Wait(&m_reqToRank, MPI_STATUS_IGNORE);
    }
    CERR << "SHUT DOWN COMPLETE" << std::endl;
    MPI_Barrier(m_comm);
    CERR << "SHUT DOWN BARRIER COMPLETE" << std::endl;
}

ClusterManager &Communicator::clusterManager() const
{
    return *m_clusterManager;
}

DataManager &Communicator::dataManager() const
{
    return *m_dataManager;
}

} // namespace vistle
