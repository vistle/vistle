#ifndef COMMUNICATOR_COLLECTIVE_H
#define COMMUNICATOR_COLLECTIVE_H

#include <vector>
#include <set>

#include <boost/asio.hpp>
#include <boost/mpi.hpp>

#include <vistle/core/message.h>
#include <vistle/core/messagepayload.h>
#include <vistle/core/availablemodule.h>

namespace vistle {

class Parameter;
class ClusterManager;
class DataManager;
class Module;

class Communicator {
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

    void setVistleRoot(const std::string &dir);
    void run();
    bool dispatch(bool *work);
    bool handleMessage(const message::Buffer &message, const MessagePayload &payload = MessagePayload());
    bool forwardToMaster(const message::Message &message, const vistle::MessagePayload &payload = MessagePayload());
    bool broadcastAndHandleMessage(const message::Message &message, const MessagePayload &payload = MessagePayload());
    bool sendMessage(int receiver, const message::Message &message, int rank = -1,
                     const MessagePayload &payload = MessagePayload());

    int hubId() const;
    int getRank() const;
    int getSize() const;

    unsigned short uiPort() const;

    ClusterManager &clusterManager() const;
    DataManager &dataManager() const;
    bool connectHub(std::string host, unsigned short port, unsigned short dataPort);
    const AvailableMap &localModules();
    boost::mpi::communicator comm() const;

    void lock();
    void unlock();

private:
    bool sendHub(const message::Message &message, const MessagePayload &payload = MessagePayload());
    bool connectData();
    bool scanModules(const std::string &dir);


    boost::mpi::communicator m_comm;
    ClusterManager *m_clusterManager;
    DataManager *m_dataManager;

    std::recursive_mutex m_mutex;
    bool isMaster() const;
    int m_hubId;
    const int m_rank;
    const int m_size;
    std::string m_vistleRoot;

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

    boost::asio::io_service m_ioService;
    boost::asio::ip::tcp::socket m_hubSocket;
    boost::asio::ip::tcp::resolver::iterator m_dataEndpoint;

    AvailableMap m_localModules;

    void setStatus(const std::string &text, int prio);
    void clearStatus();
};

} // namespace vistle

#endif
