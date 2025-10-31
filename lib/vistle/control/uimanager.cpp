/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <vistle/core/statetracker.h>
#include <vistle/core/messagequeue.h>
#include <vistle/core/tcpmessage.h>

#include "uimanager.h"
#include "uiclient.h"
#include "hub.h"

#include <boost/version.hpp>

namespace vistle {

#define CERR std::cerr << "UiManager: "

UiManager::UiManager(Hub &hub, StateTracker &stateTracker)
: m_hub(hub)
, m_stateTracker(stateTracker)
, m_locked(false)
, m_requestQuit(false)
, m_uiCount(vistle::message::Id::ModuleBase)
{}

UiManager::~UiManager()
{
    disconnect();
}

bool UiManager::handleMessage(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const message::Message &msg,
                              const buffer &payload)
{
    using namespace vistle::message;

    std::shared_ptr<UiClient> sender;
    auto it = m_clients.find(sock);
    if (it == m_clients.end()) {
        if (m_hub.verbosity() >= Hub::Verbosity::Manager) {
            CERR << "message from unknown UI" << std::endl;
        }
    } else {
        sender = it->second;
    }

    switch (msg.type()) {
    case MODULEEXIT: {
        if (!sender) {
            auto &exit = msg.as<ModuleExit>();
            if (m_hub.verbosity() >= Hub::Verbosity::Manager) {
                CERR << "unknown UI on hub " << exit.senderId() << " quit" << std::endl;
            }
        } else {
            sender->cancel();
            removeClient(sender);
        }
        return true;
    }
    case QUIT: {
        auto quit = msg.as<Quit>();
        if (quit.id() == Id::Broadcast) {
            if (!sender) {
                CERR << "unknown UI quit" << std::endl;
            } else {
                sender->cancel();
                removeClient(sender);
            }
        }
        break;
    }
    default:
        break;
    }

    if (isLocked()) {
        m_queue.emplace_back(msg);
        return true;
    }

    return m_hub.handleMessage(msg, sock, &payload);
}

void UiManager::sendMessage(const message::Message &msg, int id, const buffer *payload) const
{
    std::vector<std::shared_ptr<UiClient>> toRemove;

    std::unique_lock lock(m_mutex);
    for (auto ent: m_clients) {
        if (id == message::Id::Broadcast || ent.second->id() == id) {
            if (!sendMessage(ent.second, msg, payload)) {
                toRemove.push_back(ent.second);
            }
        }
    }

    for (auto ent: toRemove) {
        removeClient(ent);
    }
}

bool UiManager::sendMessage(std::shared_ptr<UiClient> c, const message::Message &msg, const buffer *payload) const
{
    std::unique_lock lock(m_mutex);
#if BOOST_VERSION < 107000
    //FIXME is message reliably sent, e.g. also during shutdown, without polling?
    auto &ioService = c->socket()->get_io_service();
#endif
    bool ret = message::send(*c->socket(), msg, payload);
#if BOOST_VERSION < 107000
    ioService.poll();
#endif
    return ret;
}

void UiManager::requestQuit()
{
    m_requestQuit = true;
    sendMessage(message::Quit());
}

void UiManager::disconnect()
{
    sendMessage(message::Quit());

    for (const auto &c: m_clients) {
        c.second->cancel();
    }

    std::unique_lock lock(m_mutex);
    m_clients.clear();
}

void UiManager::addClient(std::shared_ptr<boost::asio::ip::tcp::socket> sock)
{
    std::shared_ptr<UiClient> c(new UiClient(*this, m_uiCount, sock));

    std::unique_lock lock(m_mutex);
    m_clients.insert(std::make_pair(sock, c));

    if (m_hub.verbosity() >= Hub::Verbosity::Manager) {
        CERR << "new UI " << m_uiCount << " connected, now have " << m_clients.size() << " connections" << std::endl;
    }
    ++m_uiCount;

    if (m_requestQuit) {
        sendMessage(c, message::Quit());
    } else {
        sendMessage(c, message::SetId(c->id()));

        sendMessage(c, message::LockUi(true));
        auto state = m_stateTracker.getLockedState();
        for (auto &m: state.messages) {
            sendMessage(c, m.message, m.payload.get());
        }
        if (!m_locked) {
            sendMessage(c, message::LockUi(m_locked));
        }
    }
}

bool UiManager::removeClient(std::shared_ptr<UiClient> c) const
{
    if (m_hub.verbosity() >= Hub::Verbosity::Manager) {
        CERR << "removing client " << c->id() << std::endl;
    }

    std::unique_lock lock(m_mutex);
    for (auto &ent: m_clients) {
        if (ent.second == c) {
            if (!c->done()) {
                sendMessage(c, message::Quit());
                c->cancel();
            }
            m_clients.erase(ent.first);
            return true;
        }
    }

    return false;
}

void UiManager::lockUi(bool locked)
{
    if (m_locked != locked) {
        sendMessage(message::LockUi(locked));
        m_locked = locked;
    }

    if (!m_locked) {
        for (auto &m: m_queue) {
            m_hub.handleMessage(m);
        }
        m_queue.clear();
    }
}

void UiManager::lock()
{
    lockUi(true);
}

void UiManager::unlock()
{
    lockUi(false);
}

bool UiManager::isLocked() const
{
    return m_locked;
}

} // namespace vistle
