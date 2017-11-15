#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <functional>
#include <set>
#include <thread>
#include <deque>

#include <core/message.h>
#include <core/messages.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/mpi/request.hpp>
#include <boost/mpi/communicator.hpp>

namespace vistle {

class Communicator;
class StateTracker;
class Object;

class DataManager {

public:
    DataManager(boost::mpi::communicator &comm);
    ~DataManager();
    bool handle(const message::Message &msg, const std::vector<char> *payload);
    bool requestObject(const message::AddObject &add, const std::string &objId, const std::function<void()> &handler);
    bool requestObject(const std::string &referrer, const std::string &objId, int hub, int rank, const std::function<void()> &handler);
    bool requestArray(const std::string &referrer, const std::string &arrayId, int type, int hub, int rank, const std::function<void()> &handler);
    bool prepareTransfer(const message::AddObject &add);
    bool completeTransfer(const message::AddObjectCompleted &complete);
    bool connect(boost::asio::ip::tcp::resolver::iterator &hub);
    bool dispatch();

    bool send(const message::Message &message, const std::vector<char> *payload=nullptr);

private:
    bool handlePriv(const message::RequestObject &req);
    bool handlePriv(const message::SendObject &snd, const std::vector<char> *payload);

    struct Msg {
       Msg(message::Buffer &buf, std::vector<char> &payload);
       message::Buffer buf;
       std::vector<char> payload;
    };
    std::mutex m_recvMutex;
    std::deque<Msg> m_recvQueue;
    bool m_quit = false;

    void sendLoop();
    void recvLoop();

    boost::mpi::communicator m_comm;
    const int m_rank, m_size;
    boost::mpi::request m_req;
    int m_msgSize;

    boost::asio::io_service m_ioService;
    boost::asio::ip::tcp::socket m_dataSocket;

    std::thread m_recvThread, m_sendThread;

    struct AddObjectLess {
       bool operator()(const message::AddObject &a1, const message::AddObject &a2) const {
          if (a1.uuid() != a2.uuid()) {
             return a1.uuid() < a2.uuid();
          }
          if (a1.destId() != a2.destId()) {
             return a1.destId() < a2.destId();
          }
#if 0
          if (!strcmp(a1.getDestPort(), a2.getDestPort())) {
             return strcmp(a1.getDestPort(), a2.getDestPort()) < 0;
          }
#endif
          return false;
       }
    };
    std::set<message::AddObject, AddObjectLess> m_inTransitObjects; //!< objects for which AddObject messages have been sent to remote hubs

    std::map<message::AddObject, std::vector<std::string>, AddObjectLess> m_outstandingAdds; //!< AddObject messages for which requests to retrieve objects from remote have been sent
    std::map<std::string, message::AddObject> m_outstandingRequests; //!< requests for (sub-)objects which have not been serviced yet


    std::map<std::string, std::vector<std::function<void()>>> m_outstandingArrays; //!< requests for (sub-)objects which have not been serviced yet
    struct OutstandingObject {
       OutstandingObject(): obj(nullptr) {}

       vistle::Object *obj;
       std::vector<std::function<void()>> completionHandlers;
    };

    std::map<std::string, OutstandingObject> m_outstandingObjects; //!< requests for (sub-)objects which have not been serviced yet
};

} // namespace vistle
#endif
