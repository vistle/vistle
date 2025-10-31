#ifndef VISTLE_CONTROL_UIMANAGER_H
#define VISTLE_CONTROL_UIMANAGER_H

#include <map>

#include <memory>
#include <mutex>
#include <boost/asio.hpp>

#include <vistle/core/message.h>

namespace vistle {

class UiClient;
class StateTracker;
class Hub;

class UiManager {
public:
    UiManager(Hub &hub, StateTracker &stateTracker);
    ~UiManager();

    void requestQuit();
    bool handleMessage(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const message::Message &msg,
                       const buffer &payload);
    void sendMessage(const message::Message &msg, int id = message::Id::Broadcast,
                     const buffer *payload = nullptr) const;
    void addClient(std::shared_ptr<boost::asio::ip::tcp::socket> sock);

    void lock();
    void unlock();
    bool isLocked() const;

private:
    void lockUi(bool locked);
    bool sendMessage(std::shared_ptr<UiClient> c, const message::Message &msg, const buffer *payload = nullptr) const;

    void disconnect();
    bool removeClient(std::shared_ptr<UiClient> c) const;

    Hub &m_hub;
    StateTracker &m_stateTracker;

    mutable std::recursive_mutex m_mutex;
    bool m_locked;
    bool m_requestQuit;

    int m_uiCount = 0;
    mutable std::map<std::shared_ptr<boost::asio::ip::tcp::socket>, std::shared_ptr<UiClient>> m_clients;
    std::vector<message::Buffer> m_queue;
};

} // namespace vistle

#endif
