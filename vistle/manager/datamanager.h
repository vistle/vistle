#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <core/message.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/thread/mutex.hpp>

namespace vistle {

class Communicator;
class StateTracker;

class DataManager {

public:
    DataManager(int rank, int size);
    bool handle(const message::Message &msg);
    bool requestObject(const message::AddObject &add, const std::string &objId, bool array);
    bool prepareTransfer(const message::AddObject &add);
    bool completeTransfer(const message::AddObjectCompleted &complete);
    bool connect(boost::asio::ip::tcp::resolver::iterator &hub);
    bool dispatch();

    bool send(const message::Message &message);
    bool send(const char *buf, size_t n);
    bool read(char *buf, size_t n);
private:
    bool handlePriv(const message::RequestObject &req);
    bool handlePriv(const message::SendObject &snd);

    const int m_rank, m_size;
    boost::asio::io_service m_ioService;
    boost::asio::ip::tcp::socket m_dataSocket;
    boost::mutex m_dataReadMutex, m_dataWriteMutex;

    struct AddObjectCompare {
       bool operator()(const message::AddObject &a1, const message::AddObject &a2) const {
          if (a1.uuid() != a2.uuid()) {
             return a1.uuid() < a2.uuid();
          }
          if (a1.destId() != a2.destId()) {
             return a1.destId() < a2.destId();
          }
#if 0
          if (!strcmp(a1.getDestPort(), a2.getDestPort())) {
             return strcmp(a1.getDestPort(), a2.getDestPort());
          }
#endif
          return false;
       }
    };
    std::set<message::AddObject, AddObjectCompare> m_inTransitObjects; //!< objects for which AddObject messages have been sent to remote hubs
    std::map<message::AddObject, std::vector<std::string>, AddObjectCompare> m_outstandingAdds; //!< AddObject messages for which requests to retrieve objects from remote have been sent
    std::map<std::string, message::AddObject> m_outstandingRequests; //!< requests for (sub-)objects which have not been serviced yet
};

} // namespace vistle
#endif
