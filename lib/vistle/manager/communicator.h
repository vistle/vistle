#ifndef VISTLE_MANAGER_COMMUNICATOR_H
#define VISTLE_MANAGER_COMMUNICATOR_H

#include <vector>
#include <set>
#include <atomic>

#include <boost/asio.hpp>
#include <boost/mpi.hpp>

#include <vistle/core/message.h>
#include <vistle/core/messagepayload.h>
#include <vistle/core/availablemodule.h>

#include "export.h"

namespace vistle {

class Parameter;
class ClusterManager;
class DataManager;
class Module;

class V_MANAGEREXPORT Communicator {
    friend class ClusterManager;
    friend class DataManager;

public:
    enum MpiTags {
        TagToRank,
        TagStartBroadcast,
        TagData,
    };

    Communicator(int rank, const std::vector<std::string> &hosts, boost::mpi::communicator comm);
    ~Communicator();
    static Communicator &the();

    void setVistleRoot(const std::string &dir, const std::string &buildtype);
    bool run();
    bool dispatch(bool *work);
    void terminate();
    bool handleMessage(const message::Buffer &message, const MessagePayload &payload = MessagePayload());
    bool forwardToMaster(const message::Message &message, const vistle::MessagePayload &payload = MessagePayload());
    bool broadcastAndHandleMessage(const message::Message &message, const MessagePayload &payload = MessagePayload());
    bool sendMessage(int receiver, const message::Message &message, int rank = -1,
                     const MessagePayload &payload = MessagePayload());
    bool sendHub(const message::Message &message, const MessagePayload &payload = MessagePayload());

    int hubId() const;
    int getRank() const;
    int getSize() const;

    ClusterManager &clusterManager() const;
    DataManager &dataManager() const;
    bool connectHub(std::string host, unsigned short port, unsigned short dataPort);
    boost::mpi::communicator comm() const;

    void lock();
    void unlock();

private:
    bool connectData();


    boost::mpi::communicator m_comm;
    ClusterManager *m_clusterManager;
    DataManager *m_dataManager;
    std::atomic<bool> m_terminate = false;

    std::recursive_mutex m_mutex;
    bool isMaster() const;
    int m_hubId;
    const int m_rank;
    const int m_size;
    std::string m_vistleRoot;
    std::string m_buildType;

    unsigned m_recvSize;
    message::Buffer m_recvBufToRank, m_recvBufToAny;
    MPI_Request m_reqAny, m_reqToRank;
    struct SendRequest {
        SendRequest(const message::Message &msg): buf(msg) {}
        SendRequest(const message::Buffer &buf): buf(buf) {}
        message::Buffer buf;
        MessagePayload payload;
        MPI_Request req, payload_req;

        bool waitComplete();
        bool testComplete();
    };
    std::set<std::shared_ptr<SendRequest>> m_ongoingSends;
    bool startSend(int destRank, const message::Message &message, const MessagePayload &payload);

    static Communicator *s_singleton;

    Communicator(const Communicator &other); // not implemented

    boost::asio::io_context m_ioContext;
    boost::asio::ip::tcp::socket m_hubSocket;
    boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> m_dataEndpoint;

    void setStatus(const std::string &text, int prio);
    void clearStatus();
};

} // namespace vistle

#endif
