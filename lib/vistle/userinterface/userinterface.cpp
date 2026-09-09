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
, m_observer(observer)
, m_stateTracker(message::Id::UI, "UI state")
, m_socket(m_ioContext)
, m_locked(true)
{
    crypto::initialize();

    message::DefaultSender::init(message::Id::UI, 0);

    if (m_observer) {
        m_observerRegistered = m_stateTracker.registerObserver(m_observer);
        m_observer->uiLockChanged(m_locked);
    }

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
    m_ioContext.stop();

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

    asio::ip::tcp::resolver resolver(m_ioContext);
    boost::system::error_code ec;
    auto endpoints = resolver.resolve(host, std::to_string(m_remotePort), asio::ip::tcp::resolver::numeric_service, ec);
    if (ec) {
        CERR << "could not resolve " << host << ":" << m_remotePort << ": " << ec.message() << std::endl;
        m_isConnected = false;
        m_quit = true;
        return false;
    }
    asio::connect(socket(), endpoints, ec);
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
        std::shared_ptr<buffer> payload = std::make_shared<buffer>();
        message::error_code ec;
        if (!message::recv(socket(), buf, ec, true /* blocking */, payload.get())) {
            CERR << "receiving failed: " << ec.message() << std::endl;
            return false;
        }
        message::BufferEnvelope msg;
        if (payload->empty())
            msg = message::BufferEnvelope(buf);
        else
            msg = message::BufferEnvelope(buf, payload);
        if (!handleMessage(msg))
            return false;
    }

    vistle::adaptive_wait(work, this);

    return true;
}


bool UserInterface::sendMessage(const message::BufferEnvelope &msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_locked && msg.message().type() != message::IDENTIFY) {
        m_sendQueue.emplace_back(msg);
        return true;
    }
    message::error_code ec;
    return message::send(socket(), msg.message(), ec, msg.payloadData(), msg.payloadSize());
}


bool UserInterface::handleMessage(const message::BufferEnvelope &msg)
{
    bool ret = m_stateTracker.handle(msg.message(), msg.payloadData(), msg.payloadSize());

    {
        std::lock_guard<std::mutex> lock(m_messageMutex);
        MessageMap::iterator it = m_messageMap.find(const_cast<message::uuid_t &>(msg.message().uuid()));
        if (it != m_messageMap.end()) {
            it->second->buf.resize(msg.message().size());
            memcpy(it->second->buf.data(), msg.payloadData(), msg.payloadSize());
            it->second->received = true;
            it->second->cond.notify_all();
        }
    }

    switch (msg.message().type()) {
    case message::IDENTIFY: {
        auto &id = msg.as<message::Identify>();
        if (id.identity() == message::Identify::REQUEST) {
            message::Identify reply(id, message::Identify::UI);
            reply.computeMac();
            sendMessage(reply);
        }
        return true;
        break;
    }

    case message::SETID: {
        auto &id = msg.as<message::SetId>();
        m_id = id.getId();
        assert(m_id > 0);
        message::DefaultSender::init(id.senderId(), -m_id);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_initialized = true;
        //CERR << "received new UI id: " << m_id << std::endl;
        break;
    }

    case message::LOCKUI: {
        auto &lock = msg.as<message::LockUi>();
        std::lock_guard<std::mutex> guard(m_mutex);
        m_locked = lock.locked();
        if (m_observer) {
            m_observer->uiLockChanged(m_locked);
        }
        if (!m_locked) {
            for (auto &m: m_sendQueue) {
                message::error_code ec;
                message::send(socket(), msg.message(), ec, m.payloadData(), m.payloadSize());
            }
            m_sendQueue.clear();
        }
        break;
    }

    case message::QUIT: {
        auto &quit = msg.as<message::Quit>();
        (void)quit;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_quit = true;
        return false;
        break;
    }

    case message::FILEQUERYRESULT: {
        auto &fq = msg.as<message::FileQueryResult>();
        bool found = false;
        for (auto b: m_fileBrowser) {
            if (b->id() == fq.filebrowserId()) {
                b->handleMessage(msg);
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
    if (m_observer && m_observerRegistered) {
        m_stateTracker.unregisterObserver(m_observer);
        m_observer = nullptr;
        m_observerRegistered = false;
    }
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

bool FileBrowser::sendMessage(const message::BufferEnvelope &msg)
{
    assert(m_ui);
    if (msg.message().type() == message::FILEQUERY) {
        auto newMsg = msg;
        auto &fq = newMsg.as<message::FileQuery>();
        fq.setFilebrowserId(m_id);
        return m_ui->sendMessage(newMsg);
    }
    return m_ui->sendMessage(msg);
}

} // namespace vistle
