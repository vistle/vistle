#include <cstdio>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>

#include <vistle/util/hostname.h>
#include <vistle/util/sleep.h>
#include <vistle/util/crypto.h>
#include <vistle/util/sysdep.h>
#include <vistle/core/message.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/core/parameter.h>
#include <vistle/core/port.h>
#include "userinterface.h"

namespace asio = boost::asio;

#define CERR std::cerr << "UI [" << m_hostname << ":" << id() << "] "

namespace vistle {

UserInterface::UserInterface(const std::string &host, const unsigned short port, StateObserver *observer)
: m_id(-1)
, m_remoteHost(host)
, m_remotePort(port)
, m_isConnected(false)
, m_stateTracker("UI state")
, m_socket(m_ioService)
, m_locked(true)
{
    crypto::initialize();

    message::DefaultSender::init(message::Id::UI, 0);

    if (observer)
        m_stateTracker.registerObserver(observer);

    //m_stateTracker.handle(message::Trace(message::Id::Broadcast, message::ANY, true));

    m_hostname = hostname();

    CERR << "started on " << hostname() << std::endl;

    tryConnect();
}

void UserInterface::stop()
{
    if (isConnected()) {
        vistle::message::ModuleExit m;
        m.setDestId(vistle::message::Id::LocalHub);
        sendMessage(m);
    }

    cancel();
}

void UserInterface::cancel()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_initialized = false;
    }
    m_socket.cancel();
    if (isConnected()) {
        try {
            m_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
        } catch (std::exception &ex) {
            CERR << "exception during socket shutdown: " << ex.what() << std::endl;
        }
    }
    m_ioService.stop();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_quit = true;
}

int UserInterface::id() const
{
    return m_id;
}

const std::string &UserInterface::host() const
{
    return m_hostname;
}

boost::asio::ip::tcp::socket &UserInterface::socket()
{
    return m_socket;
}

const boost::asio::ip::tcp::socket &UserInterface::socket() const
{
    return m_socket;
}

bool UserInterface::tryConnect()
{
    assert(!isConnected());
    std::string host = m_remoteHost;
    if (host == m_hostname)
        host = "localhost";

    asio::ip::tcp::resolver resolver(m_ioService);
    asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(m_remotePort),
                                         asio::ip::tcp::resolver::query::numeric_service);
    boost::system::error_code ec;
    asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
    if (ec) {
        CERR << "could not resolve " << host << ":" << m_remotePort << ": " << ec.message() << std::endl;
        m_isConnected = false;
        m_quit = true;
        return false;
    }
    asio::connect(socket(), endpoint_iterator, ec);
    if (ec) {
        CERR << "could not establish connection to " << host << ":" << m_remotePort << ": " << ec.message()
             << std::endl;
        m_isConnected = false;
        if (ec == boost::system::errc::connection_refused) {
            return true;
        }
        m_quit = true;
        return false;
    }
    m_isConnected = true;
    return true;
}

bool UserInterface::isConnected() const
{
    return m_isConnected && socket().is_open();
}

StateTracker &UserInterface::state()
{
    return m_stateTracker;
}

bool UserInterface::dispatch()
{
    bool work = false;
    while (!isConnected()) {
        if (!tryConnect()) {
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_quit)
                return false;
        }
        if (isConnected())
            break;
        sleep(1);
    }

    while (isConnected()) {
        work = true;

        message::Buffer buf;
        buffer payload;
        message::error_code ec;
        if (!message::recv(socket(), buf, ec, true /* blocking */, &payload)) {
            CERR << "receiving failed: " << ec.message() << std::endl;
            return false;
        }

        if (!handleMessage(&buf, payload))
            return false;
    }

    vistle::adaptive_wait(work, this);

    return true;
}


bool UserInterface::sendMessage(const message::Message &message, const buffer *payload)
{
    if (m_locked && message.type() != message::IDENTIFY) {
        m_sendQueue.emplace_back(message, payload);
        return true;
    }

    return message::send(socket(), message, payload);
}


bool UserInterface::handleMessage(const vistle::message::Message *message, const buffer &payload)
{
    bool ret = m_stateTracker.handle(*message, payload.data(), payload.size());

    {
        std::lock_guard<std::mutex> lock(m_messageMutex);
        MessageMap::iterator it = m_messageMap.find(const_cast<message::uuid_t &>(message->uuid()));
        if (it != m_messageMap.end()) {
            it->second->buf.resize(message->size());
            memcpy(it->second->buf.data(), message, message->size());
            it->second->received = true;
            it->second->cond.notify_all();
        }
    }

    switch (message->type()) {
    case message::IDENTIFY: {
        const message::Identify *id = static_cast<const message::Identify *>(message);
        if (id->identity() == message::Identify::REQUEST) {
            message::Identify reply(*id, message::Identify::UI);
            reply.computeMac();
            sendMessage(reply);
        }
        return true;
        break;
    }

    case message::SETID: {
        const message::SetId *id = static_cast<const message::SetId *>(message);
        m_id = id->getId();
        assert(m_id > 0);
        message::DefaultSender::init(id->senderId(), -m_id);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_initialized = true;
        //CERR << "received new UI id: " << m_id << std::endl;
        break;
    }

    case message::LOCKUI: {
        auto lock = static_cast<const message::LockUi *>(message);
        m_locked = lock->locked();
        if (!m_locked) {
            for (auto &m: m_sendQueue) {
                sendMessage(m.buf, m.payload.get());
            }
            m_sendQueue.clear();
        }
        break;
    }

    case message::QUIT: {
        const message::Quit *quit = static_cast<const message::Quit *>(message);
        (void)quit;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_quit = true;
        return false;
        break;
    }

    case message::FILEQUERYRESULT: {
        auto &fq = message->as<message::FileQueryResult>();
        bool found = false;
        for (auto b: m_fileBrowser) {
            if (b->id() == fq.filebrowserId()) {
                b->handleMessage(fq, payload);
                found = true;
                break;
            }
        }
        if (!found) {
            CERR << "did not find filebrowser with id " << fq.filebrowserId() << std::endl;
        }
        break;
    }

    default:
        break;
    }

    return ret;
}

bool UserInterface::getLockForMessage(const message::uuid_t &uuid)
{
    std::lock_guard<std::mutex> lock(m_messageMutex);
    MessageMap::iterator it = m_messageMap.find(uuid);
    if (it == m_messageMap.end()) {
        it = m_messageMap.insert(std::make_pair(uuid, std::shared_ptr<RequestedMessage>(new RequestedMessage()))).first;
    }
    it->second->mutex.lock();
    //m_messageMap[const_cast<message::uuid_t &>(uuid)]->mutex.lock();
    return true;
}

bool UserInterface::getMessage(const message::uuid_t &uuid, message::Message &msg)
{
    m_messageMutex.lock();
    MessageMap::iterator it = m_messageMap.find(uuid);
    if (it == m_messageMap.end()) {
        m_messageMutex.unlock();
        return false;
    }

    if (!it->second->received) {
        std::mutex &mutex = it->second->mutex;
        std::condition_variable &cond = it->second->cond;
        std::unique_lock<std::mutex> lock(mutex, std::adopt_lock_t());

        m_messageMutex.unlock();
        cond.wait(lock);
        m_messageMutex.lock();
    }

    if (!it->second->received) {
        m_messageMutex.unlock();
        return false;
    }

    memcpy(&msg, &*it->second->buf.data(), it->second->buf.size());
    m_messageMap.erase(it);
    m_messageMutex.unlock();
    return true;
}

void UserInterface::registerObserver(StateObserver *observer)
{
    m_stateTracker.registerObserver(observer);
}

void UserInterface::registerFileBrowser(FileBrowser *browser)
{
    assert(browser->m_ui == nullptr);

    ++m_fileBrowserCount;
    browser->m_ui = this;
    browser->m_id = m_fileBrowserCount;
    m_fileBrowser.push_back(browser);
}

void UserInterface::removeFileBrowser(FileBrowser *browser)
{
    assert(browser->m_ui == this);

    auto it = std::find(m_fileBrowser.begin(), m_fileBrowser.end(), browser);
    if (it != m_fileBrowser.end()) {
        m_fileBrowser.erase(it);
        browser->m_ui = nullptr;
    }
}

UserInterface::~UserInterface()
{
    std::cerr << "  userinterface [" << host() << "] [" << id() << "] quit" << std::endl;
}

bool UserInterface::isInitialized() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

bool UserInterface::isQuitting() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_quit;
}

const std::string &UserInterface::remoteHost() const
{
    return m_remoteHost;
}

unsigned short UserInterface::remotePort() const
{
    return m_remotePort;
}

FileBrowser::~FileBrowser()
{}

int FileBrowser::id() const
{
    return m_id;
}

bool FileBrowser::sendMessage(const message::Message &message, const buffer *payload)
{
    assert(m_ui);
    message::Buffer buf(message);
    if (buf.type() == message::FILEQUERY) {
        auto &fq = buf.as<message::FileQuery>();
        fq.setFilebrowserId(m_id);
    }
    return m_ui->sendMessage(buf, payload);
}

} // namespace vistle
