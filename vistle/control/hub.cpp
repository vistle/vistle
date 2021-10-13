/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <iostream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <sstream>
#include <cassert>
#include <signal.h>

#include <vistle/util/hostname.h>
#include <vistle/util/userinfo.h>
#include <vistle/util/directory.h>
#include <vistle/util/sleep.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/crypto.h>
#include <vistle/core/message.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/core/messagerouter.h>
#include <vistle/core/porttracker.h>
#include <vistle/core/statetracker.h>
#include <vistle/core/shm.h>
#include <vistle/control/vistleurl.h>
#include <vistle/util/byteswap.h>

#ifdef MODULE_THREAD
#include <vistle/insitu/libsim/connectLibsim/connect.h>
#endif // MODULE_THREAD


#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/process.hpp>

#include "uiclient.h"
#include "hub.h"
#include "fileinfocrawler.h"
#include "scanscripts.h"
#ifdef HAVE_PYTHON
#include <vistle/userinterface/pythonmodule.h>
#include "pythoninterpreter.h"
#endif

//#define DEBUG_DISTRIBUTED

namespace asio = boost::asio;
namespace proc = boost::process;
using std::shared_ptr;
namespace dir = vistle::directory;

namespace vistle {

using message::Router;
using message::Id;

namespace Process {
enum Id {
    Manager = 0,
    Cleaner = -1,
    GUI = -2,
    Debugger = -3,
    VRB = -4,
};
}

#define CERR std::cerr << "Hub " << m_hubId << ": "

static Hub *hub_instance = nullptr;
volatile std::atomic<bool> Hub::m_interrupt(false);

void Hub::signalHandler(const boost::system::error_code &error, int signal_number)
{
    //std::cerr << "Hub: handling signal " << signal_number << std::endl;

    if (error) {
        std::cerr << "Hub: error while handling signal: " << error.message() << std::endl;
        return;
    }

    // A signal occurred.
    switch (signal_number) {
    case SIGINT:
    case SIGTERM:
        //std::cerr << "Hub: interruting because of signal" << std::endl;
        m_interrupt = true;
        break;
    }
}

Hub::Hub(bool inManager)
: m_inManager(inManager)
, m_port(0)
, m_masterPort(m_basePort)
, m_masterHost("localhost")
, m_acceptorv4(new boost::asio::ip::tcp::acceptor(m_ioService))
, m_acceptorv6(new boost::asio::ip::tcp::acceptor(m_ioService))
, m_stateTracker("Hub state")
, m_uiManager(*this, m_stateTracker)
, m_managerConnected(false)
, m_quitting(false)
, m_signals(m_ioService)
, m_isMaster(true)
, m_slaveCount(0)
, m_hubId(Id::Invalid)
, m_moduleCount(0)
, m_traceMessages(message::INVALID)
, m_execCount(0)
, m_barrierActive(false)
, m_barrierReached(0)
#if BOOST_VERSION >= 106600
, m_workGuard(asio::make_work_guard(m_ioService))
#else
, m_workGuard(new asio::io_service::work(m_ioService))
#endif
, m_ioThread([this]() { m_ioService.run(); })
{
    assert(!hub_instance);
    hub_instance = this;

    if (!inManager) {
        message::DefaultSender::init(m_hubId, 0);
    }
    make.setId(m_hubId);
    make.setRank(0);
    m_uiManager.lockUi(true);

    // install a signal handler for e.g. Ctrl-c
    m_interrupt = false;
    m_signals.add(SIGINT);
    m_signals.add(SIGTERM);
    m_signals.async_wait(signalHandler);
}

Hub::~Hub()
{
    if (!m_isMaster) {
        sendMaster(message::RemoveHub(m_hubId));
        sendMaster(message::CloseConnection("terminating"));
    }

    while (!m_sockets.empty()) {
        removeSocket(m_sockets.begin()->first);
    }

    m_dataProxy.reset();

    m_workGuard.reset();
    m_ioService.stop();
    m_ioThread.join();

    hub_instance = nullptr;
}

int Hub::run()
{
    try {
        while (dispatch())
            ;
    } catch (vistle::exception &e) {
        std::cerr << "Hub: fatal exception: " << e.what() << std::endl << e.where() << std::endl;
        return 1;
    } catch (std::exception &e) {
        std::cerr << "Hub: fatal exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

Hub &Hub::the()
{
    return *hub_instance;
}

bool Hub::init(int argc, char *argv[])
{
    try {
        if (!crypto::initialize(sizeof(message::Identify::session_data_t))) {
            std::cerr << "Hub: failed to initialize cryptographic support" << std::endl;
            return false;
        }
    } catch (const except::exception &e) {
        std::cerr << "Hub: failed to initialize cryptographic support:" << std::endl;
        std::cerr << "     " << e.what() << std::endl << e.where() << std::endl;
        return false;
    }

    m_prefix = dir::prefix(argc, argv);

    m_name = hostname();

    namespace po = boost::program_options;
    po::options_description desc("usage");
    // clang-format off
    desc.add_options()
        ("help,h", "show this message")
        ("hub,c", po::value<std::string>(), "connect to hub")
        ("batch,b", "do not start user interface")
        ("gui,g", "start graphical user interface")
        ("tui,t", "start command line interface (requires ipython)")
        ("port,p", po::value<unsigned short>(), "control port")
        ("dataport", po::value<unsigned short>(), "data port")
        ("execute,e", "call compute() after workflow has been loaded")
        ("libsim,l", po::value<std::string>(), "connect to a LibSim instrumented simulation by entering the path to the .sim2 file")
        ("exposed,gateway-host,gateway,gw", po::value<std::string>(), "ports are exposed externally on this host")
        ("url", "Vistle URL, script to process, or slave name")
    ;
    // clang-format on
    po::variables_map vm;
    try {
        po::positional_options_description popt;
        popt.add("url", 1);
        po::store(po::command_line_parser(argc, argv).options(desc).positional(popt).run(), vm);
        po::notify(vm);
    } catch (std::exception &e) {
        CERR << e.what() << std::endl;
        CERR << desc << std::endl;
        return false;
    }

    if (vm.count("help")) {
        CERR << desc << std::endl;
        return false;
    }

    if (vm.count("") > 0) {
        CERR << desc << std::endl;
        return false;
    }

    if (vm.count("port") > 0) {
        m_port = vm["port"].as<unsigned short>();
    }
    if (vm.count("dataport") > 0) {
        m_dataPort = vm["dataport"].as<unsigned short>();
    }

    if (vm.count("exposed") > 0) {
        m_exposedHost = vm["exposed"].as<std::string>();
        boost::asio::ip::tcp::resolver resolver(m_ioService);
        boost::asio::ip::tcp::resolver::query query(m_exposedHost, std::to_string(dataPort()));
        boost::system::error_code ec;
        boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query, ec);
        if (ec) {
            CERR << "could not resolve gateway host " << m_exposedHost << ": " << ec.message() << std::endl;
            return false;
        } else {
            auto endpoint = iter->endpoint();
            m_exposedHostAddr = endpoint.address();
            CERR << m_exposedHost << " resolved to " << m_exposedHostAddr << std::endl;
        }
    }

    std::string uiCmd = "vistle_gui";
    if (const char *pbs_env = getenv("PBS_ENVIRONMENT")) {
        if (std::string("PBS_INTERACTIVE") != pbs_env) {
            if (!vm.count("batch"))
                CERR << "apparently running in PBS batch mode - defaulting to no UI" << std::endl;
            uiCmd.clear();
        }
    }
    bool pythonUi = false;
    if (vm.count("tui")) {
        pythonUi = true;
        uiCmd.clear();
    }
    if (vm.count("gui")) {
        uiCmd = "vistle_gui";
    }
    if (vm.count("batch")) {
        uiCmd.clear();
        pythonUi = false;
    }

    std::string url;
    ConnectionData connectionData;
    if (vm.count("url") == 1) {
        url = vm["url"].as<std::string>();
        if (VistleUrl::parse(url, connectionData)) {
            url.clear();
            m_isMaster = connectionData.master;
            if (m_isMaster) {
                m_port = connectionData.port;
            } else {
                if (connectionData.port == 0)
                    connectionData.port = m_basePort;
                m_masterHost = connectionData.host;
                m_masterPort = connectionData.port;
            }
            if (!connectionData.hex_key.empty()) {
                CERR << "setting session key to " << connectionData.hex_key << std::endl;
                crypto::set_session_key(connectionData.hex_key);
            }
            if (connectionData.kind == "/ui" || connectionData.kind == "/gui") {
                std::string uipath = dir::bin(m_prefix) + "/" + uiCmd;
                startUi(uipath, true);
            }
        }
    }

    if (m_isMaster && vm.count("hub") > 0) {
        m_isMaster = false;
        m_masterHost = vm["hub"].as<std::string>();
        auto colon = m_masterHost.find(':');
        if (colon != std::string::npos) {
            m_masterPort = boost::lexical_cast<unsigned short>(m_masterHost.substr(colon + 1));
            m_masterHost = m_masterHost.substr(0, colon);
        }
    }

    try {
        startServer();
    } catch (std::exception &ex) {
        CERR << "failed to initialise control server on port " << m_port << ": " << ex.what() << std::endl;
        return false;
    }

    try {
        if (m_dataPort > 0) {
            m_dataProxy.reset(
                new DataProxy(m_ioService, m_stateTracker, m_dataPort ? m_dataPort : m_port + 1, !m_dataPort));
        } else {
            m_dataProxy.reset(new DataProxy(m_ioService, m_stateTracker, 0, 0));
        }
        m_dataProxy->setTrace(m_traceMessages);
    } catch (std::exception &ex) {
        CERR << "failed to initialise data server on port " << (m_dataPort ? m_dataPort : m_port + 1) << ": "
             << ex.what() << std::endl;
        return false;
    }

    if (m_isMaster) {
        // this is the master hub
        m_hubId = Id::MasterHub;
        if (!m_inManager) {
            message::DefaultSender::init(m_hubId, 0);
        }
        make.setId(m_hubId);
        make.setRank(0);
        Router::init(message::Identify::HUB, m_hubId);

        m_masterPort = m_port;
        m_dataProxy->setHubId(m_hubId);

        m_scriptPath = url;
    } else {
        if (!url.empty())
            m_name = url;

        if (!connectToMaster(m_masterHost, m_masterPort)) {
            CERR << "failed to connect to master at " << m_masterHost << ":" << m_masterPort << std::endl;
            return false;
        }
    }

    if (vm.count("execute") > 0) {
        m_executeModules = true;
    }

    // start UI
    if (!m_inManager && !m_interrupt && !m_quitting) {
        if (!uiCmd.empty()) {
            m_hasUi = true;
            std::string uipath = dir::bin(m_prefix) + "/" + uiCmd;
            startUi(uipath);
        }
        if (pythonUi) {
            m_hasUi = true;
            startPythonUi();
        }

        std::string port = boost::lexical_cast<std::string>(this->port());
        std::string dataport = boost::lexical_cast<std::string>(dataPort());

        // start manager on cluster
        std::string cmd = dir::bin(m_prefix) + "/vistle_manager";
        std::vector<std::string> args;
        args.push_back(cmd);
        args.push_back("-from-vistle");
        args.push_back(hostname());
        args.push_back(port);
        args.push_back(dataport);
#ifdef MODULE_THREAD
        if (vm.count("libsim") > 0) {
            auto sim2FilePath = vm["libsim"].as<std::string>();

            CERR << "starting manager in simulation" << std::endl;
            if (vistle::insitu::libsim::attemptLibSImConnection(sim2FilePath, args)) {
                sendInfo("Successfully connected to simulation");
            } else {
                CERR << "failed to spawn Vistle manager in the simulation" << std::endl;
                exit(1);
            }

        } else {
#endif // MODULE_THREAD
            auto child = launchMpiProcess(args);
            if (!child->valid()) {
                CERR << "failed to spawn Vistle manager " << std::endl;
                exit(1);
            }
            m_processMap[child] = Process::Manager;
#ifdef MODULE_THREAD
        }
#endif // MODULE_THREAD
    }

    if (m_isMaster && !m_inManager && !m_interrupt && !m_quitting) {
        m_hasVrb = startVrb();
    }

    return true;
}

const StateTracker &Hub::stateTracker() const
{
    return m_stateTracker;
}

StateTracker &Hub::stateTracker()
{
    return m_stateTracker;
}

unsigned short Hub::port() const
{
    return m_port;
}

unsigned short Hub::dataPort() const
{
    if (!m_dataProxy)
        return 0;

    return m_dataProxy->port();
}

std::shared_ptr<proc::child> Hub::launchMpiProcess(const std::vector<std::string> &args) const
{
    assert(!args.empty());
#ifdef _WIN32
    std::string spawn = "spawn_vistle.bat";
#else
    std::string spawn = "spawn_vistle.sh";
#endif
    auto child = std::make_shared<proc::child>(proc::search_path(spawn), proc::args(args));
#ifndef _WIN32
    if (!child->valid()) {
        std::cerr << "failed to execute " << args[0] << " via spawn_vistle.sh, retrying with mpirun" << std::endl;
        child = std::make_shared<proc::child>(proc::search_path("mpirun"), proc::args(args));
    }
#endif
    return child;
}

bool Hub::sendMessage(Hub::socket_ptr sock, const message::Message &msg, const buffer *payload)
{
    bool result = true;
    bool close = msg.type() == message::CLOSECONNECTION;

#if 0
   try {
      result = message::send(*sock, msg, payload);
   } catch(const boost::system::system_error &err) {
      CERR << "exception: err.code()=" << err.code() << std::endl;
      result = false;
      if (err.code() == boost::system::errc::broken_pipe) {
         CERR << "broken pipe" << std::endl;
         removeSocket(sock);
      } else {
         throw(err);
      }
   }
   if (close)
       removeSocket(sock);
#else
    std::shared_ptr<buffer> pl;
    if (payload)
        pl = std::make_shared<buffer>(*payload);

    message::async_send(*sock, msg, pl, [this, sock, close](boost::system::error_code ec) {
        if (ec) {
#if 0
           if (ec.code() == boost::system::errc::broken_pipe) {
               CERR << "send error, socket closed: " << ec.message() << std::endl;
               removeSocket(sock);
           }
#endif
            if (ec.value() == boost::asio::error::eof) {
                CERR << "send error, socket closed: " << ec.message() << std::endl;
                removeSocket(sock);
            }
            if (ec.value() == boost::asio::error::broken_pipe) {
                CERR << "send error, broken pipe: " << ec.message() << std::endl;
                removeSocket(sock);
            }
            CERR << "send error: " << ec.message() << std::endl;
        }
        if (close)
            removeSocket(sock);
    });
#endif
    return result;
}

bool Hub::startServer()
{
    unsigned short port = m_basePort;
    if (m_basePort == m_dataPort) {
        port = m_basePort + 1;
    }
    if (m_port) {
        port = m_port;
    }

    boost::system::error_code ec;
    while (!start_listen(port, *m_acceptorv4, *m_acceptorv6, ec)) {
        if (ec == boost::system::errc::address_in_use) {
            ++port;
        } else if (ec) {
            CERR << "failed to listen on port " << port << ": " << ec.message() << std::endl;
            return false;
        }
    }

    m_port = port;

    if (m_isMaster) {
        ConnectionData cd;
        cd.port = m_port;
        cd.host = vistle::hostname();
        cd.hex_key = crypto::get_session_key();
        auto url = VistleUrl::create(cd);

        setSessionUrl(url);
        CERR << "listening for connections on port " << m_port << std::endl;
    }

    startAccept(m_acceptorv4);
    startAccept(m_acceptorv6);

    return true;
}

bool Hub::startAccept(std::shared_ptr<acceptor> a)
{
    if (!a->is_open())
        return false;

    socket_ptr sock(new asio::ip::tcp::socket(m_ioService));
    addSocket(sock);
    a->async_accept(*sock, [this, a, sock](boost::system::error_code ec) { handleAccept(a, sock, ec); });
    return true;
}

void Hub::handleAccept(std::shared_ptr<acceptor> a, Hub::socket_ptr sock, const boost::system::error_code &error)
{
    if (error) {
        removeSocket(sock);
        return;
    }

    addClient(sock);

    auto ident = make.message<message::Identify>();
    sendMessage(sock, ident);

    startAccept(a);
}

void Hub::addSocket(Hub::socket_ptr sock, message::Identify::Identity ident)
{
    bool ok = m_sockets.insert(std::make_pair(sock, ident)).second;
    assert(ok);
    (void)ok;
}

bool Hub::removeSocket(Hub::socket_ptr sock, bool close)
{
    if (close) {
        try {
            sock->shutdown(asio::ip::tcp::socket::shutdown_both);
            sock->close();
        } catch (std::exception &ex) {
            CERR << "closing socket failed: " << ex.what() << std::endl;
        }
    }

    if (m_isMaster) {
        for (auto &s: m_slaves) {
            if (s.second.sock == sock) {
                CERR << "lost connection to slave " << s.first << std::endl;
                s.second.sock.reset();
            }
        }
        for (auto &s: m_vrbSockets) {
            if (s.second == sock) {
                CERR << "lost connection to VRB for module " << s.first << std::endl;
                m_vrbSockets.erase(s.first);
                break;
            }
        }
    } else {
        if (m_masterSocket == sock) {
            CERR << "lost connection to master" << std::endl;
            m_masterSocket.reset();
        }
    }

    if (removeClient(sock)) {
        CERR << "removed client" << std::endl;
    }

    bool ret = m_sockets.erase(sock) > 0;
    sock.reset();
    return ret;
}

void Hub::addClient(Hub::socket_ptr sock)
{
    //CERR << "new client" << std::endl;
    m_clients.insert(sock);
}

bool Hub::removeClient(Hub::socket_ptr sock)
{
    return m_clients.erase(sock) > 0;
}

void Hub::addSlave(const std::string &name, Hub::socket_ptr sock)
{
    assert(m_isMaster);
    ++m_slaveCount;
    const int slaveid = Id::MasterHub - m_slaveCount;
    m_slaves[slaveid].sock = sock;
    m_slaves[slaveid].name = name;
    m_slaves[slaveid].ready = false;
    m_slaves[slaveid].id = slaveid;

    if (m_ready) {
        auto set = make.message<message::SetId>(slaveid);
        sendMessage(sock, set);
    } else {
        m_slavesToConnect.push_back(&m_slaves[slaveid]);
    }
}

void Hub::removeSlave(int id)
{
    m_slaves.erase(id);
}

void Hub::slaveReady(Slave &slave)
{
    CERR << "slave hub '" << slave.name << "' ready" << std::endl;

    assert(m_isMaster);
    assert(!slave.ready);

    auto state = m_stateTracker.getState();
    for (auto &m: state) {
        sendMessage(slave.sock, m.message, m.payload.get());
    }
    slave.ready = true;
}

bool Hub::dispatch()
{
    bool ret = true;
    bool work = false;
    size_t avail = 0;
    do {
        socket_ptr sock;
        avail = 0;
        for (auto &s: m_clients) {
            if (!s->is_open()) {
                CERR << "socket closed" << std::endl;
                removeSocket(sock);
                return true;
            }
            boost::asio::socket_base::bytes_readable command(true);
            try {
                s->io_control(command);
            } catch (std::exception &ex) {
                CERR << "socket error: " << ex.what() << std::endl;
                removeSocket(sock);
                return true;
            }
            if (command.get() >= avail) {
                avail = command.get();
                sock = s;
            }
        }
        boost::system::error_code error;
        if (sock) {
            work = true;
#if 0
            if (avail == 0) {
                CERR << "socket closed" << std::endl;
                removeSocket(sock);
                return true;
            }
#endif
            handleWrite(sock, error);
        }
    } while (!m_interrupt && !m_quitting && avail > 0);

    if (m_interrupt) {
        if (m_isMaster) {
            sendSlaves(message::Quit());
            sendSlaves(message::CloseConnection("user interrupt"));
        }
        m_workGuard.reset();
        for (unsigned count = 0; count < 300; ++count) {
            if (m_ioService.stopped())
                break;
            usleep(10000);
        }
        emergencyQuit();
    }

    if (!m_isMaster && !m_masterSocket) {
        emergencyQuit();
    }

    if (checkChildProcesses()) {
        work = true;
    }

    if (m_quitting) {
        if (!hasChildProcesses(true)) {
            ret = false;
        } else {
#if 0
          CERR << "still " << m_processMap.size() << " processes running" << std::endl;
          for (const auto &proc: m_processMap) {
              std::cerr << "   id: " << proc.second << ", pid: " << proc.first << std::endl;
          }
#endif
        }
    }

    vistle::adaptive_wait(work);

    return ret;
}

void Hub::handleWrite(Hub::socket_ptr sock, const boost::system::error_code &error)
{
    if (error) {
        CERR << "error during write on socket: " << error.message() << std::endl;
        removeSocket(sock);
        //m_quitting = true;
        return;
    }

    message::Identify::Identity senderType = message::Identify::UNKNOWN;
    auto it = m_sockets.find(sock);
    if (it != m_sockets.end())
        senderType = it->second;

    if (senderType == message::Identify::VRB) {
        handleVrb(sock);
        return;
    }

    message::Buffer msg;
    buffer payload;
    message::error_code ec;
    if (message::recv(*sock, msg, ec, false, &payload)) {
        bool ok = true;
        if (senderType == message::Identify::UI) {
            ok = m_uiManager.handleMessage(sock, msg, payload);
        } else if (senderType == message::Identify::LOCALBULKDATA) {
            //ok = handleLocalData(msg, sock);
            CERR << "invalid identity on socket: " << senderType << std::endl;
            ok = false;
        } else if (senderType == message::Identify::REMOTEBULKDATA) {
            //ok = handleRemoteData(msg, sock);
            CERR << "invalid identity on socket: " << senderType << std::endl;
            ok = false;
        } else {
            ok = handleMessage(msg, sock, &payload);
        }
        if (!ok) {
            m_quitting = true;
            //break;
        }
    } else if (ec) {
        CERR << "error during read from socket: " << ec.message() << std::endl;
        removeSocket(sock);
        //m_quitting = true;
    }
}

bool Hub::sendMaster(const message::Message &msg, const buffer *payload)
{
    assert(!m_isMaster);
    if (m_isMaster) {
        return false;
    }

    int numSent = 0;
    for (auto &sock: m_sockets) {
        if (sock.second == message::Identify::HUB) {
            if (!sendMessage(sock.first, msg, payload)) {
                removeSocket(sock.first);
                break;
            }
            ++numSent;
        }
    }
    assert(numSent <= 1);
    return numSent == 1;
}

bool Hub::sendManager(const message::Message &msg, int hub, const buffer *payload)
{
    if (hub == Id::LocalHub || hub == m_hubId || (hub == Id::MasterHub && m_isMaster)) {
        assert(m_managerConnected);
        if (!m_managerConnected) {
            CERR << "sendManager: no connection, cannot send " << msg << std::endl;
            return false;
        }

        int numSent = 0;
        for (auto &sock: m_sockets) {
            if (sock.second == message::Identify::MANAGER) {
                if (!sendMessage(sock.first, msg, payload))
                    break;
                ++numSent;
            }
        }
        return numSent == 1;
    }

    return sendHub(hub, msg, payload);
}

bool Hub::sendSlaves(const message::Message &msg, bool returnToSender, const buffer *payload)
{
    assert(m_isMaster);
    if (!m_isMaster)
        return false;

    int senderHub = msg.senderId();
    if (senderHub > 0) {
        //std::cerr << "mod id " << senderHub;
        senderHub = m_stateTracker.getHub(senderHub);
        //std::cerr << " -> hub id " << senderHub << std::endl;
    }

    bool ok = true;
    for (auto &slave: m_slaves) {
        if (slave.first != senderHub || returnToSender) {
            //std::cerr << "to slave id: " << sock.first << " (!= " << senderHub << ")" << std::endl;
            if (slave.second.ready && slave.second.sock) {
                if (!sendMessage(slave.second.sock, msg, payload))
                    ok = false;
            }
        }
    }
    return ok;
}

bool Hub::sendHub(int hub, const message::Message &msg, const buffer *payload)
{
    if (hub == m_hubId)
        return true;

    if (!m_isMaster && hub == Id::MasterHub) {
        return sendMaster(msg, payload);
    }

    for (auto &slave: m_slaves) {
        if (slave.first == hub) {
            return sendMessage(slave.second.sock, msg, payload); // FIXME
        }
    }

    return false;
}

bool Hub::sendUi(const message::Message &msg, int id, const buffer *payload)
{
    m_uiManager.sendMessage(msg, id, payload);
    return true;
}

bool Hub::sendModule(const message::Message &msg, int id, const buffer *payload)
{
    if (!Id::isModule(id)) {
        CERR << "sendModule: id " << id << " is not for a module" << std::endl;
        return false;
    }

    int hub = idToHub(id);
    if (Id::Invalid == hub) {
        CERR << "sendModule: could not find hub for id " << id << std::endl;
        return false;
    }

    if (hub == m_hubId)
        return sendManager(msg, hub, payload);

    return sendHub(hub, msg, payload);
}

int Hub::idToHub(int id) const
{
    if (id >= Id::ModuleBase)
        return m_stateTracker.getHub(id);

    if (id == Id::LocalManager || id == Id::LocalHub)
        return m_hubId;

    return m_stateTracker.getHub(id);
}

int Hub::id() const
{
    return m_hubId;
}

bool Hub::hubReady()
{
    assert(m_managerConnected);
    if (m_isMaster) {
        m_ready = true;

        for (auto s: m_slavesToConnect) {
            auto set = make.message<message::SetId>(s->id);
            sendMessage(s->sock, set);
        }
        m_slavesToConnect.clear();

        if (!processScript()) {
            auto q = message::Quit();
            sendSlaves(q);
            sendManager(q);
            m_quitting = true;
            return false;
        }
    } else {
        auto hub = make.message<message::AddHub>(m_hubId, m_name);
        hub.setNumRanks(m_localRanks);
        hub.setDestId(Id::ForBroadcast);
        hub.setPort(m_port);
        hub.setDataPort(m_dataProxy->port());
        hub.setLoginName(vistle::getLoginName());
        hub.setRealName(vistle::getRealName());
        hub.setHasUserInterface(m_hasUi);

        for (auto &sock: m_sockets) {
            if (sock.second == message::Identify::HUB) {
                if (m_exposedHost.empty()) {
                    try {
                        auto addr = sock.first->local_endpoint().address();
                        if (addr.is_v6()) {
                            hub.setAddress(addr.to_v6());
                            CERR << "AddHub: local v6 address: " << addr.to_v6() << std::endl;
                        } else if (addr.is_v4()) {
                            hub.setAddress(addr.to_v4());
                            CERR << "AddHub: local v4 address: " << addr.to_v4() << std::endl;
                        } else {
                            hub.setAddress(addr);
                            CERR << "AddHub: local address: " << addr << std::endl;
                        }
                    } catch (std::bad_cast &except) {
                        CERR << "AddHub: failed to convert local address to v6: " << except.what() << std::endl;
                        return false;
                    }
                } else {
                    hub.setAddress(m_exposedHostAddr);
                    CERR << "AddHub: exposed host: " << m_exposedHostAddr << std::endl;
                }
            }
        }

        m_stateTracker.handle(hub, nullptr, true);

        if (!sendMaster(hub)) {
            return false;
        }
        m_ready = true;
    }
    processStartupScripts();
    return true;
}

bool Hub::handleMessage(const message::Message &recv, Hub::socket_ptr sock, const buffer *payload)
{
    using namespace vistle::message;

    message::Buffer buf(recv);
    Message &msg = buf;
    message::Identify::Identity senderType = message::Identify::UNKNOWN;
    auto it = m_sockets.find(sock);
    if (sock) {
        assert(it != m_sockets.end());
        if (it != m_sockets.end())
            senderType = it->second;
    }

    if (senderType == Identify::UI)
        msg.setSenderId(m_hubId);

    switch (msg.type()) {
    case message::TRACE: {
        auto &mm = msg.as<Trace>();
        CERR << "handle Trace: " << mm << std::endl;
        if (mm.module() == m_hubId || !(Id::isHub(mm.module()) || Id::isModule(mm.module()))) {
            if (mm.on())
                m_traceMessages = mm.messageType();
            else
                m_traceMessages = message::INVALID;
            m_dataProxy->setTrace(m_traceMessages);
        }
        break;
    }
    case message::CLOSECONNECTION: {
        auto &mm = msg.as<CloseConnection>();
        CERR << "remote closes socket: " << mm.reason() << std::endl;
        removeSocket(sock);
        if (senderType == Identify::HUB || senderType == Identify::MANAGER) {
            CERR << "terminating." << std::endl;
            emergencyQuit();
            m_quitting = true;
            return false;
        }
        return true;
        break;
    }
    default: {
        break;
    }
    }

    if (msg.destId() == Id::ForBroadcast) {
        if (m_isMaster) {
            msg.setDestId(Id::Broadcast);
        } else {
            return sendMaster(msg, payload);
        }
    }

    switch (msg.type()) {
    case message::IDENTIFY: {
        auto &id = static_cast<const Identify &>(msg);
        CERR << "ident msg: " << id.identity() << std::endl;
        if (id.identity() == Identify::REQUEST) {
            if (senderType == Identify::REMOTEBULKDATA) {
                Identify reply(id, Identify::REMOTEBULKDATA, m_hubId);
                reply.computeMac();
                sendMessage(sock, reply);
            } else if (senderType == Identify::LOCALBULKDATA) {
                Identify reply(id, Identify::LOCALBULKDATA, -1);
                reply.computeMac();
                sendMessage(sock, reply);
            } else if (m_isMaster) {
                Identify reply(id, Identify::HUB, m_name);
                reply.computeMac();
                sendMessage(sock, reply);
            } else {
                Identify reply(id, Identify::SLAVEHUB, m_name);
                reply.computeMac();
                sendMessage(sock, reply);
            }
            break;
        }
        if (id.identity() != Identify::UNKNOWN && id.identity() != Identify::REQUEST) {
            it->second = id.identity();
        }
        if (!id.verifyMac()) {
            CERR << "message authentication failed" << std::endl;
            auto close = make.message<message::CloseConnection>("message authentication failed");
            sendMessage(sock, close);
            break;
        }
        switch (id.identity()) {
        case Identify::MANAGER: {
            assert(!m_managerConnected);
            m_managerConnected = true;
            m_localRanks = id.numRanks();
            m_dataProxy->setNumRanks(id.numRanks());
            m_dataProxy->setBoostArchiveVersion(id.boost_archive_version());
            CERR << "manager connected with " << m_localRanks << " ranks" << std::endl;

            if (m_hubId == Id::MasterHub) {
                auto master = make.message<message::AddHub>(m_hubId, m_name);
                master.setNumRanks(m_localRanks);
                master.setPort(m_port);
                master.setDataPort(m_dataProxy->port());
                master.setLoginName(vistle::getLoginName());
                master.setRealName(vistle::getRealName());
                if (!m_exposedHost.empty()) {
                    master.setAddress(m_exposedHostAddr);
                }
                master.setHasUserInterface(m_hasUi);
                master.setHasVrb(m_hasVrb);
                CERR << "MASTER HUB: " << master << std::endl;
                m_stateTracker.handle(master, nullptr);
                sendUi(master);
            }

            if (m_hubId != Id::Invalid) {
                auto set = make.message<message::SetId>(m_hubId);
                sendMessage(sock, set);
                if (m_hubId <= Id::MasterHub) {
                    auto state = m_stateTracker.getState();
                    for (auto &m: state) {
                        sendMessage(sock, m.message, m.payload.get());
                    }
                }
                if (!hubReady()) {
                    m_uiManager.lockUi(false);
                    return false;
                }
            }
            m_uiManager.lockUi(false);
            break;
        }
        case Identify::UI: {
            m_uiManager.addClient(sock);
            break;
        }
        case Identify::HUB: {
            if (m_isMaster) {
                CERR << "refusing connection from other master hub" << std::endl;
                removeSocket(sock);
                return true;
            }
            assert(!m_isMaster);
            CERR << "master hub connected" << std::endl;
            break;
        }
        case Identify::SLAVEHUB: {
            if (!m_isMaster) {
                CERR << "refusing connection from other slave hub, connect directly to a master" << std::endl;
                removeSocket(sock);
                return true;
            }
            assert(m_isMaster);
            CERR << "slave hub '" << id.name() << "' connected" << std::endl;
            addSlave(id.name(), sock);
            break;
        }
        case Identify::LOCALBULKDATA:
        case Identify::REMOTEBULKDATA: {
            m_dataProxy->addSocket(id, sock);
            removeClient(sock);
            break;
        }
        default: {
            CERR << "invalid identity: " << id.identity() << std::endl;
            break;
        }
        }
        return true;
    }
    case message::ADDHUB: {
        auto &mm = static_cast<const AddHub &>(msg);
        auto add = mm;
        CERR << "received AddHub: " << add << std::endl;
        if (m_isMaster) {
            auto it = m_slaves.find(add.id());
            if (it == m_slaves.end()) {
                std::cerr << "ignoring for unknown slave: " << msg << std::endl;
                break;
            }
            auto &slave = it->second;
            slaveReady(slave);
        } else {
            if (add.id() == Id::MasterHub) {
                CERR << "received AddHub for master with " << add.numRanks() << " ranks";
                if (add.hasAddress()) {
                    std::cerr << ", address is " << add.address() << std::endl;
                } else {
                    add.setAddress(m_masterSocket->remote_endpoint().address());
                    std::cerr << ", using address " << add.address() << std::endl;
                }
            }
        }
        if (m_dataProxy->connectRemoteData(add, [this]() { return dispatch(); })) {
            m_stateTracker.handle(add, nullptr, true);
            sendUi(add);
        } else if (add.id() != message::Id::MasterHub) {
            m_stateTracker.handle(add, nullptr, true);
            sendUi(add);
        } else {
            CERR << "could not establish data connection to hub " << add.address() << ":" << add.port() << " - ignoring"
                 << std::endl;
        }
        break;
    }
    case message::CONNECT: {
        //CERR << "handling connect: " << msg << std::endl;
        auto &mm = static_cast<const Connect &>(msg);
        if (m_isMaster) {
#if 0
             if (mm.isNotification()) {
                 CERR << "discarding notification on master: " << msg << std::endl;
                 return true;
             }
#endif
            if (m_stateTracker.handleConnect(mm)) {
                handlePriv(mm);
                handleQueue();
            } else {
                //CERR << "delaying connect: " << msg << std::endl;
                m_queue.emplace_back(msg);
                return true;
            }
        } else {
            if (mm.isNotification()) {
                m_stateTracker.handle(mm, nullptr);
                sendManager(mm);
                sendUi(mm);
            } else {
                sendMaster(mm);
            }
            return true;
        }
        break;
    }
    case message::DISCONNECT: {
        auto &mm = static_cast<const Disconnect &>(msg);
        if (m_isMaster) {
#if 0
             if (mm.isNotification()) {
                 CERR << "discarding notification on master: " << msg << std::endl;
                 return true;
             }
#endif
            if (m_stateTracker.handleDisconnect(mm)) {
                handlePriv(mm);
                handleQueue();
            } else {
                m_queue.emplace_back(msg);
                return true;
            }
        } else {
            if (mm.isNotification()) {
                m_stateTracker.handle(mm, nullptr);
                sendManager(mm);
                sendUi(mm);
            } else {
                sendMaster(mm);
            }
            return true;
        }
        break;
    }
    case message::MODULEAVAILABLE: {
        if (!payload || payload->empty()) {
            std::cerr << "missing payload for MODULEAVAILABLE message!" << std::endl;
            break;
        }
        AvailableModule mod(msg, *payload);
        if (mod.hub() == Id::Invalid && senderType == Identify::MANAGER)
            mod.setHub(m_hubId);
        if (mod.hub() == Id::Invalid) {
            CERR << "invalid module message from " << senderType << ": " << mod.print() << std::endl;
        } else {
            AvailableModule::Key key(mod.hub(), mod.name());
            auto modIt = m_availableModules.emplace(key, std::move(mod));
            modIt.first->second.send([this](const message::Message &avail, const buffer *pl) {
                bool reverseRetval = false;
                reverseRetval += !m_stateTracker.handle(avail, pl);
                if (!m_isMaster)
                    reverseRetval += !sendMaster(avail, pl);
                reverseRetval += !sendUi(avail, Id::Broadcast, pl);
                return reverseRetval;
            });
        }
        return true;
        break;
    }
    case message::CREATEMODULECOMPOUND: {
        if (!payload) {
            std::cerr << "missing payload for CREATEMODULECOMPOUND message!" << std::endl;
            break;
        }
        ModuleCompound comp(msg, *payload);
#ifdef HAVE_PYTHON
        moduleCompoundToFile(comp);
#endif
        comp.send(
            std::bind(&Hub::sendManager, this, std::placeholders::_1, message::Id::LocalHub, std::placeholders::_2));
    }
    default:
        break;
    }

    const int dest = idToHub(msg.destId());
    const int sender = idToHub(msg.senderId());

    {
        bool mgr = false, ui = false, master = false, slave = false;

        if (m_hubId == Id::MasterHub) {
            if (msg.destId() == Id::ForBroadcast)
                msg.setDestId(Id::Broadcast);
        }
        bool track = Router::the().toTracker(msg, senderType) && msg.type() != message::ADDHUB;
        m_stateTracker.handle(msg, payload ? payload->data() : nullptr, payload ? payload->size() : 0, track);

        if (Router::the().toManager(msg, senderType, sender) || (Id::isModule(msg.destId()) && dest == m_hubId)) {
            sendManager(msg, Id::LocalHub, payload);
            mgr = true;
        }
        if (Router::the().toUi(msg, senderType)) {
            sendUi(msg, Id::Broadcast, payload);
            ui = true;
        }
        if (Id::isModule(msg.destId()) && !Id::isHub(dest)) {
            CERR << "failed to translate module id " << msg.destId() << " to hub" << std::endl;
        } else if (!Id::isHub(dest)) {
            if (Router::the().toMasterHub(msg, senderType, sender)) {
                sendMaster(msg, payload);
                master = true;
            }
            if (Router::the().toSlaveHub(msg, senderType, sender)) {
                sendSlaves(msg, msg.destId() == Id::Broadcast /* return to sender */, payload);
                slave = true;
            }
            assert(!(slave && master));
        } else {
            if (dest != m_hubId) {
                if (m_isMaster) {
                    sendHub(dest, msg, payload);
                    //sendSlaves(msg);
                    slave = true;
                } else if (sender == m_hubId) {
                    sendMaster(msg, payload);
                    master = true;
                }
            }
        }

        if (m_traceMessages == message::ANY || msg.type() == m_traceMessages || msg.type() == message::SPAWN
#ifdef DEBUG_DISTRIBUTED
            || msg.type() == message::ADDOBJECT || msg.type() == message::ADDOBJECTCOMPLETED
#endif
        ) {
            if (track)
                std::cerr << "t";
            else
                std::cerr << ".";
            if (mgr)
                std::cerr << "m";
            else
                std::cerr << ".";
            if (ui)
                std::cerr << "u";
            else
                std::cerr << ".";
            if (slave) {
                std::cerr << "s";
            } else if (master) {
                std::cerr << "M";
            } else
                std::cerr << ".";
            std::cerr << " " << msg << std::endl;
        }
    }

    if (Router::the().toHandler(msg, senderType)) {
        switch (msg.type()) {
        case message::SPAWN: {
            auto &spawn = static_cast<const Spawn &>(msg);
            handlePriv(spawn);
            break;
        }

        case message::SPAWNPREPARED: {
            auto &spawn = static_cast<const SpawnPrepared &>(msg);
            if (spawn.isNotification()) {
                auto it = m_sendAfterSpawn.find(spawn.spawnId());
                if (it != m_sendAfterSpawn.end()) {
                    for (auto &m: it->second) {
                        handleMessage(m);
                    }
                    m_sendAfterSpawn.erase(it);
                }
            } else {
#ifndef MODULE_THREAD
                assert(spawn.hubId() == m_hubId);
                assert(m_ready);

                const AvailableModule *mod = findModule({spawn.hubId(), spawn.getName()}, spawn.spawnId());
                if (!mod)
                    return true;
                spawnModule(mod->path(), spawn.getName(), spawn.spawnId());
#endif
            }
            break;
        }

        case message::DEBUG: {
            auto &debug = msg.as<message::Debug>();
#ifdef MODULE_THREAD
            int id = 0;
#else
            int id = debug.getModule();
#endif
            bool launched = false;
            for (auto &p: m_processMap) {
                if (p.second == id) {
                    std::vector<std::string> args;
                    std::stringstream str;
                    str << "-attach-mpi=" << p.first;
                    args.push_back(str.str());
                    auto child = std::make_shared<proc::child>(proc::search_path("ddt"), proc::args(args));
                    std::stringstream info;
                    info << "Launched ddt as PID " << child->id() << ", attaching to " << p.first;
                    sendInfo(info.str());
                    m_processMap[child] = Process::Debugger;
                    launched = true;
                    break;
                }
            }
            if (!launched) {
                std::stringstream str;
                str << "Did not find PID to debug module id " << debug.getModule();
#ifdef MODULE_THREAD
                str << " -> " << id;
#endif
                sendError(str.str());
            }
            break;
        }

        case message::SETID: {
            assert(!m_isMaster);
            auto &set = static_cast<const SetId &>(msg);
            m_hubId = set.getId();
            if (!m_inManager) {
                message::DefaultSender::init(m_hubId, 0);
            }
            make.setId(m_hubId);
            make.setRank(0);
            Router::init(message::Identify::SLAVEHUB, m_hubId);
            CERR << "got hub id " << m_hubId << std::endl;
            m_dataProxy->setHubId(m_hubId);
            if (m_managerConnected) {
                sendManager(set);
                for (auto &mod: m_localModules) {
                    mod.setHub(m_hubId);
                    mod.send([this](const message::Message &avail, const buffer *payload) {
                        auto reversetRetval = !m_stateTracker.handle(avail, payload);
                        reversetRetval += !sendMaster(avail, payload);
                        reversetRetval += !sendUi(avail, message::Id::Broadcast, payload);
                        return !reversetRetval;
                    });
                }
                m_localModules.clear();
            }
            if (m_managerConnected) {
#if 0
               auto state = m_stateTracker.getState();
               for (auto &m: state) {
                  sendMessage(sock, m);
               }
#endif
                if (!hubReady()) {
                    return false;
                }
            }
            break;
        }

        case message::QUIT: {
            auto &quit = static_cast<const Quit &>(msg);
            handlePriv(quit, senderType);
            break;
        }

        case message::REMOVEHUB: {
            auto &rm = static_cast<const RemoveHub &>(msg);
            handlePriv(rm);
            break;
        }

        case message::EXECUTE: {
            auto &exec = static_cast<const Execute &>(msg);
            handlePriv(exec);
            break;
        }

        case message::CANCELEXECUTE: {
            auto &cancel = static_cast<const CancelExecute &>(msg);
            handlePriv(cancel);
            break;
        }

        case message::BARRIER: {
            auto &barr = static_cast<const Barrier &>(msg);
            handlePriv(barr);
            break;
        }

        case message::BARRIERREACHED: {
            auto &reached = static_cast<const BarrierReached &>(msg);
            handlePriv(reached);
            break;
        }

        case message::REQUESTTUNNEL: {
            auto &tunnel = static_cast<const RequestTunnel &>(msg);
            handlePriv(tunnel);
            break;
        }

        case message::ADDPORT: {
            handleQueue();
            break;
        }
        case FILEQUERY: {
            auto &query = msg.as<FileQuery>();
            handlePriv(query, payload);
            break;
        }
        case FILEQUERYRESULT: {
            auto &result = msg.as<FileQueryResult>();
            handlePriv(result, payload);
            break;
        }
        case COVER: {
            auto &cover = msg.as<Cover>();
            handlePriv(cover, payload);
            break;
        }
        case MODULEEXIT: {
            auto &exit = msg.as<ModuleExit>();
            handlePriv(exit);
            break;
        }
        default: {
            break;
        }
        }
    }

    return true;
}

bool Hub::handlePriv(const message::Spawn &spawn)
{
    if (m_isMaster) {
        bool restart = Id::isModule(spawn.migrateId());
        bool isMirror = Id::isModule(spawn.mirroringId());
        bool shouldMirror = !restart && !isMirror && std::string(spawn.getName()) == "COVER";
        auto notify = spawn;
        notify.setReferrer(spawn.uuid());
        notify.setSenderId(m_hubId);
        bool doSpawn = false;
        int mirroredId = Id::Invalid;
        if (spawn.spawnId() == Id::Invalid) {
            notify.setSpawnId(Id::ModuleBase + m_moduleCount);
            mirroredId = Id::ModuleBase + m_moduleCount;
            ++m_moduleCount;
            doSpawn = true;
        }
        if (restart) {
            if (doSpawn) {
                cacheModuleValues(spawn.migrateId(), notify.spawnId());
            }
            killOldModule(spawn.migrateId());
        }
        CERR << "sendManager: " << notify << std::endl;
        notify.setDestId(Id::Broadcast);
        if (shouldMirror)
            notify.setMirroringId(mirroredId);
        m_stateTracker.handle(notify, nullptr);
        sendManager(notify);
        sendUi(notify);
        sendSlaves(notify, true /* return to sender */);
        if (doSpawn) {
            notify.setDestId(spawn.hubId());
            CERR << "doSpawn: sendManager: " << notify << std::endl;
            sendManager(notify, spawn.hubId());
        }

        if (doSpawn && shouldMirror) {
            for (auto &hubid: m_stateTracker.getHubs()) {
                if (hubid == spawn.hubId())
                    continue;
                const auto &hub = m_stateTracker.getHubData(hubid);
                if (!hub.hasUi)
                    continue;

                message::Spawn mirror(hubid, spawn.getName());
                //mirror.setReferrer(spawn.uuid());
                //mirror.setSenderId(m_hubId);
                mirror.setSpawnId(Id::ModuleBase + m_moduleCount);
                ++m_moduleCount;
                mirror.setMirroringId(mirroredId);
                mirror.setDestId(Id::Broadcast);
                m_stateTracker.handle(mirror, nullptr);
                CERR << "sendManager mirror: " << mirror << std::endl;
                sendManager(mirror);
                sendUi(mirror);
                sendSlaves(mirror, true /* return to sender */);
                mirror.setDestId(hubid);
                CERR << "doSpawn: sendManager mirror: " << mirror << std::endl;
                sendManager(mirror, spawn.hubId());
            }
        }
    } else {
        CERR << "SLAVE: handle spawn: " << spawn << std::endl;
        if (spawn.spawnId() == Id::Invalid) {
            sendMaster(spawn);
        } else {
            m_stateTracker.handle(spawn, nullptr);
            sendManager(spawn);
            sendUi(spawn);
        }
    }

    return true;
}

bool Hub::handleQueue()
{
    using namespace message;

    //CERR << "unqueuing " << m_queue.size() << " messages" << std::endl;;

    bool again = true;
    while (again) {
        again = false;
        decltype(m_queue) queue;
        for (auto &m: m_queue) {
            if (m.type() == message::CONNECT) {
                auto &mm = m.as<Connect>();
                if (m_stateTracker.handleConnect(mm)) {
                    again = true;
                    handlePriv(mm);
                } else {
                    queue.push_back(m);
                }
            } else if (m.type() == message::DISCONNECT) {
                auto &mm = m.as<Disconnect>();
                if (m_stateTracker.handleDisconnect(mm)) {
                    again = true;
                    handlePriv(mm);
                } else {
                    queue.push_back(m);
                }
            } else {
                std::cerr << "message other than Connect/Disconnect in queue: " << m << std::endl;
                queue.push_back(m);
            }
        }
        std::swap(m_queue, queue);
    }

    return true;
}

bool Hub::startCleaner()
{
#if defined(MODULE_THREAD) && defined(NO_SHMEM)
    return true;
#endif

#ifdef SHMDEBUG
    CERR << "SHMDEBUG: preserving shm for session " << Shm::instanceName(hostname(), m_port) << std::endl;
    return true;
#endif

    if (getenv("SLURM_JOB_ID")) {
        CERR << "not starting under clean_vistle - running SLURM" << std::endl;
        return false;
    }

    // run clean_vistle on cluster
    std::string cmd = dir::bin(m_prefix) + "/clean_vistle";
    std::vector<std::string> args;
    args.push_back(cmd);
    std::string shmname = Shm::instanceName(hostname(), m_port);
    args.push_back(shmname);
    auto child = launchMpiProcess(args);
    if (!child->valid()) {
        CERR << "failed to spawn clean_vistle" << std::endl;
        return false;
    }
    m_processMap[child] = Process::Cleaner;
    return true;
}

void Hub::cacheModuleValues(int oldModuleId, int newModuleId)
{
    using namespace vistle::message;
    assert(Id::isModule(oldModuleId));
    auto paramNames = m_stateTracker.getParameters(oldModuleId);
    for (const auto &pn: paramNames) {
        auto p = m_stateTracker.getParameter(oldModuleId, pn);
        auto pm = SetParameter(newModuleId, p->getName(), p);
        pm.setDestId(newModuleId);
        m_sendAfterSpawn[newModuleId].emplace_back(pm);
    }
    auto inputs = m_stateTracker.portTracker()->getConnectedInputPorts(oldModuleId);
    for (const auto &in: inputs) {
        for (const auto &from: in->connections()) {
            auto cm = Connect(from->getModuleID(), from->getName(), newModuleId, in->getName());
            m_sendAfterSpawn[newModuleId].emplace_back(cm);
        }
    }
    auto outputs = m_stateTracker.portTracker()->getConnectedOutputPorts(oldModuleId);
    for (const auto &out: outputs) {
        for (const auto &to: out->connections()) {
            auto cm = Connect(newModuleId, out->getName(), to->getModuleID(), to->getName());
            m_sendAfterSpawn[newModuleId].emplace_back(cm);
        }
    }
}

void Hub::killOldModule(int migratedId)
{
    assert(Id::isModule(migratedId));
    message::Kill kill(migratedId);
    kill.setDestId(migratedId);
    handleMessage(kill);
}


void Hub::sendInfo(const std::string &s)
{
    CERR << s << std::endl;
    auto t = make.message<message::SendText>(message::SendText::Info);
    message::SendText::Payload pl(s);
    auto payload = addPayload(t, pl);
    sendUi(t, Id::Broadcast, &payload);
}

void Hub::sendError(const std::string &s)
{
    CERR << "Error: " << s << std::endl;
    auto t = make.message<message::SendText>(message::SendText::Error);
    message::SendText::Payload pl(s);
    auto payload = addPayload(t, pl);
    sendUi(t, Id::Broadcast, &payload);
}

void Hub::setLoadedFile(const std::string &file)
{
    CERR << "Loaded file: " << file << std::endl;
    auto t = make.message<message::UpdateStatus>(message::UpdateStatus::LoadedFile, file);
    m_stateTracker.handle(t, nullptr);
    sendUi(t);
}

void Hub::setSessionUrl(const std::string &url)
{
    std::cerr << "Share this: " << url << std::endl;
    std::cerr << std::endl;
    auto t = make.message<message::UpdateStatus>(message::UpdateStatus::SessionUrl, url);
    m_stateTracker.handle(t, nullptr);
    sendUi(t);
}

void Hub::setStatus(const std::string &s, message::UpdateStatus::Importance prio)
{
    CERR << "Status: " << s << std::endl;
    auto t = make.message<message::UpdateStatus>(s, prio);
    m_stateTracker.handle(t, nullptr);
    sendUi(t);
}

void Hub::clearStatus()
{
    m_statusText.clear();
}

bool Hub::connectToMaster(const std::string &host, unsigned short port)
{
    assert(!m_isMaster);

    CERR << "connecting to master at " << host << ":" << port << std::flush;
    bool connected = false;
    boost::system::error_code ec;
    while (!connected) {
        asio::ip::tcp::resolver resolver(m_ioService);
        asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
        m_masterSocket.reset(new boost::asio::ip::tcp::socket(m_ioService));
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
        if (ec) {
            break;
        }
        asio::connect(*m_masterSocket, endpoint_iterator, ec);
        if (!ec) {
            connected = true;
        } else if (ec == boost::system::errc::connection_refused) {
            std::cerr << "." << std::flush;
            sleep(1);
        } else {
            break;
        }
    }

    if (!connected) {
        std::cerr << " FAILED: " << ec.message() << std::endl;
        return false;
    }

    std::cerr << " done." << std::endl;
#if 0
   message::Identify ident(message::Identify::SLAVEHUB, m_name);
   sendMessage(m_masterSocket, ident);
#endif
    addSocket(m_masterSocket, message::Identify::HUB);
    addClient(m_masterSocket);

    return true;
}

bool Hub::startVrb()
{
    m_vrbPort = 0;

    proc::ipstream out;
    auto child = std::make_shared<proc::child>(proc::search_path("vrb"), proc::args({"--tui", "--printport"}),
                                               proc::std_out > out);
    if (!child->valid()) {
        CERR << "could not create VRB process" << std::endl;
        return false;
    }

    CERR << "started VRB process" << std::endl;

    std::string line;
    while (child->running() && std::getline(out, line) && !line.empty()) {
        m_vrbPort = std::atol(line.c_str());
    }
    if (m_vrbPort == 0) {
        CERR << "could not parse VRB port" << std::endl;
        child->terminate();
        return false;
    }
    CERR << "VRB process running on port " << m_vrbPort << ", running=" << child->running() << std::endl;

    m_processMap[child] = Process::VRB;
    return true;
}

Hub::socket_ptr Hub::connectToVrb(unsigned short port)
{
    assert(m_isMaster);

    socket_ptr sock;
    if (m_vrbPort == 0) {
        CERR << "cannot connect to local VRB on unknown port" << std::flush;
        return sock;
    }

    CERR << "connecting to local VRB at port " << port << std::flush;
    bool connected = false;
    boost::system::error_code ec;
    while (!connected) {
        asio::ip::tcp::resolver resolver(m_ioService);
        asio::ip::tcp::resolver::query query("localhost", boost::lexical_cast<std::string>(port));
        sock = std::make_shared<socket>(m_ioService);
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
        if (ec) {
            std::cerr << ": resolver failed: " << ec.message() << std::endl;
            return sock;
        }
        asio::connect(*sock, endpoint_iterator, ec);
        if (!ec) {
            connected = true;
        } else if (ec == boost::system::errc::connection_refused) {
            std::cerr << "." << std::flush;
            sleep(1);
        } else {
            std::cerr << ": connect failed: " << ec.message() << std::endl;
            return sock;
        }
    }

    std::cerr << " done." << std::endl;

    const char DF_IEEE = 1;
    char df = 0;
    asio::read(*sock, asio::mutable_buffer(&df, 1), ec);
    if (ec) {
        CERR << "failed to read data format" << std::endl;
        sock.reset();
        return sock;
    }
    if (df != DF_IEEE) {
        CERR << "incompatible data format " << static_cast<int>(df) << std::endl;
        sock.reset();
        return sock;
    }
    asio::write(*sock, asio::const_buffer(&DF_IEEE, 1), ec);
    if (ec) {
        CERR << "failed to write data format" << std::endl;
        sock.reset();
        return sock;
    }

    addSocket(sock, message::Identify::VRB);
    addClient(sock);

    return sock;
}

bool Hub::handleVrb(Hub::socket_ptr sock)
{
    if (!sock || !sock->is_open())
        return false;

    // cf. covise/src/kernel/net/covise_connect.cpp: Connection::sendMessage
    std::array<uint32_t, 4> header; // sender id, sender type, msg type, msg length

    boost::asio::socket_base::bytes_readable command(true);
    try {
        sock->io_control(command);
    } catch (std::exception &ex) {
        CERR << "handleVrb: socket error: " << ex.what() << std::endl;
        removeSocket(sock);
        return false;
    }
    if (command.get() < sizeof(header)) {
        CERR << "handleVrb: only have " << command.get() << " bytes" << std::endl;
        // try again later when full header is available
        return true;
    }

    boost::asio::mutable_buffer hbuf(header.data(), header.size() * sizeof(header[0]));
    boost::system::error_code ec;
    boost::asio::read(*sock, hbuf, ec);
    if (ec) {
        CERR << "handleVrb: failed to read header: " << ec.message() << std::endl;
        removeSocket(sock);
        return false;
    }
    for (auto &v: header)
        v = byte_swap<network_endian, host_endian>(v);

    buffer data(header[3]);
    boost::asio::mutable_buffer dbuf(data.data(), data.size());
    boost::asio::read(*sock, dbuf, ec);
    if (ec) {
        CERR << "handleVrb: failed to read data: " << ec.message() << std::endl;
        removeSocket(sock);
        return false;
    }

    for (auto &s: m_vrbSockets) {
        if (s.second == sock) {
            int destMod = s.first;
            int mir = m_stateTracker.getMirrorId(destMod);
            message::Cover cover(mir, header[0], header[1], header[2]);
            cover.setDestId(destMod);
            cover.setPayloadSize(data.size());
            //CERR << "handleVrb: " << cover << std::endl;
            return sendModule(cover, destMod, &data);
        }
    }

    return false;
}

bool Hub::startUi(const std::string &uipath, bool replace)
{
    std::string port = boost::lexical_cast<std::string>(this->m_masterPort);

    std::vector<std::string> args;
    if (!replace)
        args.push_back("-from-vistle");
    args.push_back(m_masterHost);
    args.push_back(port);
    if (replace) {
        boost::process::system(uipath, proc::args(args));
        exit(0);
        return false;
    }

    auto child = std::make_shared<proc::child>(uipath, proc::args(args));
    if (!child->valid()) {
        CERR << "failed to spawn UI " << uipath << std::endl;
        return false;
    }

    m_processMap[child] = Process::GUI;

    return true;
}

bool Hub::startPythonUi()
{
    std::string port = boost::lexical_cast<std::string>(this->m_masterPort);

    std::string ipython = "ipython";
    std::vector<std::string> args;
    args.push_back("-i");
    args.push_back("-c");
    std::string cmd = "import vistle; ";
    cmd += "vistle._vistle.sessionConnect(None, \"" + m_masterHost + "\", " + port + "); ";
    cmd += "from vistle import *; ";
    args.push_back(cmd);
    args.push_back("--");
    auto child = std::make_shared<proc::child>(proc::search_path(ipython), proc::args(args));
    if (!child->valid()) {
        CERR << "failed to spawn ipython " << ipython << std::endl;
        return false;
    }

    m_processMap[child] = Process::GUI;

    return true;
}


bool Hub::processScript()
{
    if (!m_scriptPath.empty()) {
        auto retval = processScript(m_scriptPath, m_executeModules);
        setLoadedFile(m_scriptPath);
        return retval;
    }
    return true;
}

bool Hub::processScript(const std::string &filename, bool executeModules)
{
    assert(m_uiManager.isLocked());
#ifdef HAVE_PYTHON
    setStatus("Loading " + m_scriptPath + "...");
    PythonInterpreter inter(filename, dir::share(m_prefix), executeModules);
    bool interrupt = false;
    while (!interrupt && inter.check()) {
        if (!dispatch())
            interrupt = true;
    }
    if (interrupt || inter.error()) {
        setStatus("Loading " + m_scriptPath + " failed");
        return false;
    }
    setStatus("Loading " + m_scriptPath + " done");
    return true;
#else
    return false;
#endif
}

bool Hub::handlePriv(const message::Quit &quit, message::Identify::Identity senderType)
{
    if (quit.id() == Id::Broadcast) {
        CERR << "quit requested by " << senderType << std::endl;
        m_uiManager.requestQuit();
        if (senderType == message::Identify::MANAGER) {
            if (m_isMaster)
                sendSlaves(quit);
        } else if (senderType == message::Identify::HUB) {
            sendManager(quit);
        } else if (senderType == message::Identify::UI) {
            if (m_isMaster)
                sendSlaves(quit);
            else
                sendMaster(quit);
            sendManager(quit);
        } else {
            sendSlaves(quit);
            sendManager(quit);
        }
        m_quitting = true;
        return false;
    } else if (quit.id() == m_hubId) {
        if (m_isMaster) {
            CERR << "ignoring request to remove master hub" << std::endl;
        } else {
            CERR << "removal of hub requested by " << senderType << std::endl;
            m_uiManager.requestQuit();
            if (senderType != message::Identify::MANAGER) {
                sendManager(quit);
            }
            m_quitting = true;
            return true;
        }
    } else if (Id::isHub(quit.id())) {
        return sendHub(quit.id(), quit);
    }

    return true;
}

bool Hub::handlePriv(const message::RemoveHub &rm)
{
    int hub = rm.id();
    if (m_isMaster) {
        if (hub != Id::MasterHub && message::Id::isHub(hub)) {
            removeSlave(hub);
            return sendHub(hub, message::Quit());
        } else {
            return false;
        }
    }

    return false;
}
bool Hub::processStartupScripts()
{
    bool error = false;
    for (const auto &script: scanStartupScripts()) {
        std::cerr << "loading script " << script << std::endl;
        error += !processScript(script, false);
    }
    return error;
}

bool Hub::handlePriv(const message::Execute &exec)
{
    auto toSend = make.message<message::Execute>(exec);
    if (exec.getExecutionCount() > m_execCount)
        m_execCount = exec.getExecutionCount();
    if (exec.getExecutionCount() < 0)
        toSend.setExecutionCount(++m_execCount);

    if (Id::isModule(exec.getModule())) {
        const int hub = m_stateTracker.getHub(exec.getModule());
        toSend.setDestId(exec.getModule());
        sendManager(toSend, hub);
    } else {
        // execute all sources in dataflow graph
        for (auto &mod: m_stateTracker.runningMap) {
            int id = mod.first;
            if (!Id::isModule(id))
                continue;
            int hub = mod.second.hub;
            auto inputs = m_stateTracker.portTracker()->getInputPorts(id);
            bool isSource = true;
            for (auto &input: inputs) {
                if (!input->connections().empty())
                    isSource = false;
            }
            if (isSource) {
                toSend.setModule(id);
                toSend.setDestId(id);
                sendManager(toSend, hub);
            }
        }
    }

    return true;
}

bool Hub::handlePriv(const message::CancelExecute &cancel)
{
    auto toSend = make.message<message::CancelExecute>(cancel);

    if (Id::isModule(cancel.getModule())) {
        const int hub = m_stateTracker.getHub(cancel.getModule());
        toSend.setDestId(cancel.getModule());
        sendManager(toSend, hub);
    }

    return true;
}

bool Hub::handlePriv(const message::Barrier &barrier)
{
    CERR << "Barrier request: " << barrier.uuid() << " by " << barrier.senderId() << std::endl;
    assert(!m_barrierActive);
    assert(m_barrierReached == 0);
    m_barrierActive = true;
    m_barrierUuid = barrier.uuid();
    message::Buffer buf(barrier);
    buf.setDestId(Id::NextHop);
    if (m_isMaster)
        sendSlaves(buf, true);
    sendManager(buf);
    return true;
}

bool Hub::handlePriv(const message::BarrierReached &reached)
{
    ++m_barrierReached;
    CERR << "Barrier " << reached.uuid() << " reached by " << reached.senderId() << " (now " << m_barrierReached << ")"
         << std::endl;
    assert(m_barrierActive);
    assert(m_barrierUuid == reached.uuid());
    // message must be received from local manager and each slave
    if (m_isMaster) {
        if (m_barrierReached == m_slaves.size() + 1) {
            m_barrierActive = false;
            m_barrierReached = 0;
            auto r = make.message<message::BarrierReached>(reached.uuid());
            r.setDestId(Id::NextHop);
            m_stateTracker.handle(r, nullptr);
            sendUi(r);
            sendSlaves(r);
            sendManager(r);
        }
    } else {
        if (reached.senderId() == Id::MasterHub) {
            m_barrierActive = false;
            m_barrierReached = 0;
            sendUi(reached);
            sendManager(reached);
        }
    }
    return true;
}

bool Hub::handlePriv(const message::RequestTunnel &tunnel)
{
    return m_tunnelManager.processRequest(tunnel);
}

bool Hub::handlePriv(const message::Connect &conn)
{
    auto c = make.message<message::Connect>(conn);
    c.setNotify(true);
    sendUi(c);
    sendManager(c);
    sendSlaves(c);

    auto modA = conn.getModuleA();
    auto modB = conn.getModuleB();
    auto mirA = m_stateTracker.getMirrors(modA);
    auto mirB = m_stateTracker.getMirrors(modB);

    for (auto a: mirA) {
        if (a == modA)
            continue;
        c.setModuleA(a);
        sendUi(c);
        sendManager(c);
        sendSlaves(c);
    }
    c.setModuleA(modA);

    for (auto b: mirB) {
        if (b == modB)
            continue;
        c.setModuleB(b);
        sendUi(c);
        sendManager(c);
        sendSlaves(c);
    }
    c.setModuleB(modB);

    return true;
}

bool Hub::handlePriv(const message::Disconnect &disc)
{
    auto d = make.message<message::Disconnect>(disc);
    d.setNotify(true);
    sendUi(d);
    sendManager(d);
    sendSlaves(d);

    auto modA = disc.getModuleA();
    auto modB = disc.getModuleB();
    auto mirA = m_stateTracker.getMirrors(modA);
    auto mirB = m_stateTracker.getMirrors(modB);

    for (auto a: mirA) {
        if (a == modA)
            continue;
        d.setModuleA(a);
        sendUi(d);
        sendManager(d);
        sendSlaves(d);
    }
    d.setModuleA(modA);

    for (auto b: mirB) {
        if (b == modB)
            continue;
        d.setModuleB(b);
        sendUi(d);
        sendManager(d);
        sendSlaves(d);
    }
    d.setModuleB(modB);

    return true;
}

bool Hub::handlePriv(const message::FileQuery &query, const buffer *payload)
{
    int hub = m_stateTracker.getHub(query.moduleId());
    if (hub != m_hubId)
        return sendHub(hub, query, payload);

    buffer pl;
    if (!payload)
        payload = &pl;

    std::cerr << "FileQuery(" << query.command() << ") for " << query.moduleId() << ":" << query.path() << std::endl;
    FileInfoCrawler c(*this);
    return c.handle(query, *payload);
}

bool Hub::handlePriv(const message::FileQueryResult &result, const buffer *payload)
{
    if (result.destId() == m_hubId)
        return sendUi(result, result.destUiId(), payload);

    return sendHub(result.destId(), result, payload);
}

bool Hub::handlePriv(const message::ModuleExit &exit)
{
    auto it = m_vrbSockets.find(exit.senderId());
    if (it != m_vrbSockets.end()) {
        removeSocket(it->second);
    }
    return true;
}

bool Hub::handlePriv(const message::Cover &cover, const buffer *payload)
{
    if (!m_isMaster) {
        //CERR << "forwarding: " << cover << std::endl;
        return sendMaster(cover, payload);
    }

    //CERR << "handling: " << cover << std::endl;

    auto mid = cover.mirrorId();
    auto mirrors = m_stateTracker.getMirrors(mid);
    socket_ptr sock;
    auto it = m_vrbSockets.find(cover.senderId());
    if (it == m_vrbSockets.end()) {
        CERR << "connecting to VRB on behalf of " << cover.senderId() << std::endl;
        sock = connectToVrb(m_vrbPort);
        if (sock) {
            m_vrbSockets.emplace(cover.senderId(), sock);
        }
    } else {
        sock = it->second;
    }
    if (sock) {
        std::array<uint32_t, 4> header; // sender id, sender type, msg type, msg length
        header[0] = cover.sender();
        header[1] = cover.senderType();
        header[2] = cover.subType();
        header[3] = payload->size();
        for (auto &v: header)
            v = byte_swap<host_endian, network_endian>(v);

        boost::asio::const_buffer hbuf(header.data(), header.size() * sizeof(header[0]));
        boost::asio::const_buffer dbuf(payload->data(), payload->size());
        std::vector<boost::asio::const_buffer> buffers{hbuf, dbuf};
        boost::system::error_code ec;
        asio::write(*sock, buffers, ec);
        if (ec) {
            removeSocket(sock);
            return false;
        }
    }

    return true;
}

bool Hub::hasChildProcesses(bool ignoreGui)
{
    if (ignoreGui) {
        for (const auto &pk: m_processMap) {
            if (pk.second != Process::GUI && pk.second != Process::VRB)
                return true;
        }

        return false;
    }

    return !m_processMap.empty();
}

bool Hub::checkChildProcesses(bool emergency)
{
    for (auto it = m_processMap.begin(), next = it; it != m_processMap.end(); it = next) {
        next = it;
        ++next;

        if (it->first->running())
            continue;

        const int id = it->second;
        std::string idstring;
        switch (id) {
        case Process::Manager:
            idstring = "Manager";
            break;
        case Process::Cleaner:
            idstring = "Cleaner";
            break;
        case Process::Debugger:
            idstring = "Debugger";
            break;
        case Process::GUI:
            idstring = "GUI";
            break;
        case Process::VRB:
            idstring = "VRB";
            break;
        default:
            idstring = "module " + std::to_string(id);
            break;
        }
        CERR << "process with id " << idstring << " (PID " << it->first->id() << ") exited" << std::endl;
        next = m_processMap.erase(it);

        if (id == Process::VRB) {
            m_vrbPort = 0;
            while (!m_vrbSockets.empty()) {
                removeSocket(m_vrbSockets.begin()->second);
            }
        } else if (!m_quitting && !emergency) {
            if (id == Process::Manager) {
                // manager died
                CERR << "manager died - cannot continue" << std::endl;
                emergencyQuit();
            }
            if (message::Id::isModule(id) && m_stateTracker.getModuleState(id) != StateObserver::Unknown &&
                m_stateTracker.getModuleState(id) != StateObserver::Quit) {
                // synthesize ModuleExit message for crashed modules
                auto m = make.message<message::ModuleExit>();
                m.setSenderId(id);
                sendManager(m); // will be returned and forwarded to master hub
            }
        }
    }

    return true;
}

void Hub::emergencyQuit()
{
    if (m_emergency)
        return;

    CERR << "forced to quit" << std::endl;
    m_emergency = true;

    m_workGuard.reset();
    m_ioService.stop();
    while (!m_ioService.stopped()) {
        usleep(100000);
    }

    m_dataProxy.reset();

    if (!m_quitting) {
        m_uiManager.requestQuit();
        m_processMap.clear();
        if (startCleaner()) {
            m_quitting = true;
            while (hasChildProcesses()) {
                if (!checkChildProcesses(true)) {
                    usleep(100000);
                }
            }
        }
        exit(1);
    }
}

const AvailableModule *Hub::findModule(const AvailableModule::Key &key, int spawnId)
{
    const AvailableModule *mod = nullptr;
    auto it = m_availableModules.find(key);
    if (it != m_availableModules.end()) {
        mod = &it->second;
    }
    if (!mod) {
        if (key.hub == m_hubId) {
            std::stringstream str;
            str << "refusing to spawn " << key.name << ":" << spawnId << ": not in list of available modules";
            sendError(str.str());
            auto ex = make.message<message::ModuleExit>();
            ex.setSenderId(spawnId);
            sendManager(ex);
        }
    }
    return mod;
}


void Hub::spawnModule(const std::string &path, const std::string &name, int spawnId)
{
    std::vector<std::string> argv;
    argv.push_back(path);
    argv.push_back(Shm::instanceName(hostname(), m_port));
    argv.push_back(name);
    argv.push_back(boost::lexical_cast<std::string>(spawnId));
    std::cerr << "starting module " << name << std::endl;
    auto child = launchMpiProcess(argv);
    if (child->valid()) {
        //CERR << "started " << mod->path() << " with PID " << pid << std::endl;
        m_processMap[child] = spawnId;
    } else {
        std::stringstream str;
        str << "program " << argv[0] << " failed to start";
        sendError(str.str());
        auto ex = make.message<message::ModuleExit>();
        ex.setSenderId(spawnId);
        sendManager(ex);
    }
}


} // namespace vistle
