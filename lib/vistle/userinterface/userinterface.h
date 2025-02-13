#ifndef VISTLE_USERINTERFACE_USERINTERFACE_H
#define VISTLE_USERINTERFACE_USERINTERFACE_H

#include <condition_variable>
#include <exception>
#include <iostream>
#include <list>
#include <map>
#include <mutex>

#include "export.h"

#include <vistle/core/message.h>
#include <vistle/core/parameter.h>
#include <vistle/core/port.h>
#include <vistle/core/porttracker.h>
#include <vistle/core/statetracker.h>
#include <vistle/util/buffer.h>

#include <boost/asio.hpp>

namespace vistle {

class UserInterface;

class V_UIEXPORT FileBrowser {
    friend class UserInterface;

public:
    virtual ~FileBrowser();
    int id() const;

    bool sendMessage(const message::Message &message, const buffer *payload = nullptr);
    virtual bool handleMessage(const message::Message &message, const buffer &payload) = 0;

private:
    UserInterface *m_ui = nullptr;
    int m_id = -1;
};

class V_UIEXPORT UserInterface {
public:
    UserInterface(const std::string &host, const unsigned short port, StateObserver *observer = nullptr);
    virtual ~UserInterface();

    bool isInitialized() const;
    bool isQuitting() const;

    void stop();
    void cancel();

    virtual bool dispatch();
    bool sendMessage(const message::Message &message, const buffer *payload = nullptr);

    int id() const;
    const std::string &host() const;

    const std::string &remoteHost() const;
    unsigned short remotePort() const;

    boost::asio::ip::tcp::socket &socket();
    const boost::asio::ip::tcp::socket &socket() const;

    bool tryConnect();
    bool isConnected() const;

    StateTracker &state();

    bool getLockForMessage(const message::uuid_t &uuid);
    bool getMessage(const message::uuid_t &uuid, message::Message &msg);

    void registerObserver(StateObserver *observer);
    void registerFileBrowser(FileBrowser *browser);
    void removeFileBrowser(FileBrowser *browser);

protected:
    int m_id;
    std::string m_hostname;
    std::string m_remoteHost;
    unsigned short m_remotePort;
    bool m_isConnected = false;
    bool m_quit = false;

    StateObserver *m_observer = nullptr;
    StateTracker m_stateTracker;

    bool handleMessage(const message::Message *message, const buffer &payload);

    boost::asio::io_context m_ioContext;
    boost::asio::ip::tcp::socket m_socket;

    struct RequestedMessage {
        std::mutex mutex;
        std::condition_variable cond;
        bool received;
        buffer buf;

        RequestedMessage(): received(false) {}
    };

    typedef std::map<message::uuid_t, std::shared_ptr<RequestedMessage>> MessageMap;
    MessageMap m_messageMap;
    std::mutex m_messageMutex; //< protect access to m_messageMap
    bool m_locked = false;
    struct MessageWithPayload {
        MessageWithPayload(const message::Message &msg, const buffer *payload = nullptr): buf(msg)
        {
            if (payload)
                this->payload.reset(new buffer(*payload));
        }
        message::Buffer buf;
        std::unique_ptr<const buffer> payload;
    };
    std::vector<MessageWithPayload> m_sendQueue;
    mutable std::mutex m_mutex;
    bool m_initialized = false;

    int m_fileBrowserCount = 0;
    std::vector<FileBrowser *> m_fileBrowser;
};

} // namespace vistle

#endif
