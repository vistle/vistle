/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <iostream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <sstream>
#include <cassert>
#include <future>
#include <condition_variable>
#include <signal.h>
#include <string>
#include <limits>
#ifdef VISTLE_USE_FMT
#include <fmt/format.h>
#include <fmt/args.h>
#endif

#include <vistle/util/hostname.h>
#include <vistle/util/userinfo.h>
#include <vistle/util/directory.h>
#include <vistle/util/filesystem.h>
#include <vistle/util/sleep.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/crypto.h>
#include <vistle/util/threadname.h>
#include <vistle/util/url.h>
#include <vistle/util/version.h>
#include <vistle/core/message.h>
#include <vistle/core/message/colormap.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/core/messagerouter.h>
#include <vistle/core/porttracker.h>
#include <vistle/core/statetracker.h>
#include <vistle/core/shm.h>
#include <vistle/control/vistleurl.h>
#include <vistle/util/byteswap.h>

#include <vistle/config/value.h>
#include <vistle/config/array.h>
#include <vistle/config/access.h>

#ifdef MODULE_THREAD
#include <vistle/insitu/libsim/connectLibsim/connect.h>
#endif // MODULE_THREAD


#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/program_options.hpp>
#ifdef __linux__
#include <sys/prctl.h>
#endif

#include "uiclient.h"
#include "hub.h"
#include "fileinfocrawler.h"
#include "scanscripts.h"
#ifdef HAVE_PYTHON
#include <vistle/python/pythonmodule.h>
#include "pythoninterpreter.h"
#endif

//#define DEBUG_DISTRIBUTED

namespace asio = boost::asio;
using std::shared_ptr;
namespace dir = vistle::directory;

namespace vistle {

using message::Router;
using message::Id;

namespace message {
bool operator<(const message::AddHub &a1, const message::AddHub &a2)
{
    return a1.id() < a2.id();
}
} // namespace message

namespace Process {
enum Id {
    Unknown = 0,
    Module = -1,
    Manager = -2,
    Cleaner = -3,
    GUI = -4,
    Debugger = -5,
    VRB = -6,
};
}

namespace {
#ifdef _WIN32
const std::string platform = "windows";
const std::string platform_fallback;
#else
const std::string platform_fallback = "unix";
#ifdef __APPLE__
const std::string platform = "darwin";
#else
const std::string platform = "linux";
#endif
#endif
} // namespace

struct terminate_with_parent: process::extend::handler {
#ifdef BOOST_POSIX_API
    template<typename Executor>
    void on_exec_setup(Executor & /*ex*/)
    {
#ifdef __linux__
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1) {
            std::cerr << "Error when setting parent-death signal: " << strerror(errno) << std::endl;
        }
#endif
    }
#endif
};

static std::function<void(int, const std::error_code &)> exit_handler;

namespace {

std::istream &getline_multi_delim(std::istream &is, std::string &str, std::string delim,
                                  size_t max_length = std::string().max_size(), bool includeDelimiter = true)
{
    char ch;
    str.clear();
    size_t length = 0;
    while (is.get(ch) && delim.find(ch) == std::string::npos) {
        str.push_back(ch);
        ++length;
        if (length >= max_length)
            return is;
    }
    if (is && includeDelimiter)
        str.push_back(ch);
    return is;
}

} // namespace

Hub::ObservedChild::ObservedChild() = default;
Hub::ObservedChild::~ObservedChild()
{
    if (outThread && outThread->joinable())
        outThread->join();
    if (errThread && errThread->joinable())
        errThread->join();
}
void Hub::ObservedChild::sendTextToUi(message::SendText::TextType stream, size_t num, const std::string &line,
                                      int moduleId) const
{
    auto t = hub->make.message<message::SendText>(stream, num);
    t.setSenderId(moduleId);
    message::SendText::Payload pl(line);
    auto payload = addPayload(t, pl);
    if (hub->isPrincipal()) {
        hub->sendUi(t, message::Id::Broadcast, &payload);
    } else {
        t.setDestId(Id::MasterHub);
        t.setDestUiId(message::Id::Broadcast);
        hub->sendMaster(t, &payload);
    }
}
void Hub::ObservedChild::sendOutputToUi(bool console) const
{
    using message::SendText;
    auto lock = std::unique_lock(mutex);
    if (numDiscarded > 0) {
        std::ostringstream str;
        str << "[" << numDiscarded << " lines of output discarded]";
        hub->sendInfo(str.str(), moduleId);
    }
    size_t count = numDiscarded, startLine = count;
    std::string text;
    SendText::TextType type = SendText::Cout;
    for (const auto &line: buffer) {
        if (text.empty() || type == line.type) {
            text += line.line;
        } else {
            lock.unlock();
            sendTextToUi(type, startLine, text, moduleId);
            startLine = count;
            type = line.type;
            text = line.line;
            lock.lock();
        }
        ++count;
    }
    lock.unlock();
    if (console) {
        std::cerr << text;
    }
    sendTextToUi(type, startLine, text, moduleId);
}
bool Hub::ObservedChild::isOutputStreaming() const
{
    auto lock = std::unique_lock(mutex);
    return streamOutput;
}
void Hub::ObservedChild::setOutputStreaming(bool enable)
{
    auto lock = std::unique_lock(mutex);
    streamOutput = enable;
}

#define CERR \
    std::cerr << "Hub" << (message::Id::isHub(m_hubId) ? " " + std::to_string(message::Id::MasterHub - m_hubId) : "") \
              << ": "

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

ConfigParameters::ConfigParameters(Hub &hub): ParameterManager("Settings", message::Id::Config), m_hub(hub)
{}

void ConfigParameters::sendParameterMessage(const message::Message &message, const buffer *payload) const
{
    message::Buffer buf(message);
    buf.setSenderId(id());
    buf.setDestId(message::Id::Broadcast);
    m_hub.stateTracker().handle(buf, payload ? payload->data() : nullptr, payload ? payload->size() : 0, true);
    m_hub.sendAll(buf, payload);
}

SessionParameters::SessionParameters(Hub &hub): ParameterManager("Vistle", message::Id::Vistle), m_hub(hub)
{}

void SessionParameters::sendParameterMessage(const message::Message &message, const buffer *payload) const
{
    message::Buffer buf(message);
    buf.setSenderId(message::Id::Vistle);
    buf.setDestId(message::Id::Broadcast);
    m_hub.stateTracker().handle(buf, payload ? payload->data() : nullptr, payload ? payload->size() : 0, true);
    m_hub.sendAll(buf, payload);
}

HubParameters::HubParameters(Hub &hub): ParameterManager("Hub", hub.id()), m_hub(hub)
{}

void HubParameters::sendParameterMessage(const message::Message &message, const buffer *payload) const
{
    message::Buffer buf(message);
    buf.setSenderId(id());
    buf.setDestId(message::Id::Broadcast);
    m_hub.stateTracker().handle(buf, payload ? payload->data() : nullptr, payload ? payload->size() : 0, true);
    m_hub.sendAll(buf, payload);
}

Hub::Hub(bool inManager)
: m_stateTracker(message::Id::Invalid, "Hub state")
, m_inManager(inManager)
, m_basePort(31093)
, m_port(0)
, m_masterPort(m_basePort)
, m_masterHost("localhost")
, m_acceptorv4(new boost::asio::ip::tcp::acceptor(m_ioContext))
, m_acceptorv6(new boost::asio::ip::tcp::acceptor(m_ioContext))
, m_uiManager(*this, m_stateTracker)
, m_managerConnected(false)
, m_quitting(false)
, m_signals(m_ioContext)
, m_isMaster(true)
, m_slaveCount(0)
, m_hubId(Id::Invalid)
, m_moduleCount(0)
, m_traceMessages(message::INVALID)
, m_barrierActive(false)
, m_workGuard(asio::make_work_guard(m_ioContext))
, session(*this)
, settings(*this)
, params(*this)
{
    assert(!hub_instance);
    hub_instance = this;

    if (!inManager) {
        message::DefaultSender::init(m_hubId, 0);
#ifdef __APPLE__
        for (const auto *var: {
                 "DYLD_LIBRARY_PATH",
                 "DYLD_FRAMEWORK_PATH",
                 "DYLD_FALLBACK_LIBRARY_PATH",
                 "DYLD_FALLBACK_FRAMEWORK_PATH",
             }) {
            if (const auto *path = getenv(var)) {
                std::string vvar("VISTLE_");
                vvar += var;
                setenv(vvar.c_str(), path, 1);
            }
        }
#endif
    }

    make.setId(m_hubId);
    make.setRank(0);
    m_uiManager.lockUi(true);

    for (int i = 0; i < 4; ++i)
        startIoThread();

    // install a signal handler for e.g. Ctrl-c
    m_interrupt = false;
    m_signals.add(SIGINT);
    m_signals.add(SIGTERM);
    m_signals.async_wait(signalHandler);

    exit_handler = [this](int exit_code, const std::error_code &ec) {
#if 0
        if (ec) {
            std::cerr << "error during child exit: " << ec.message() << std::endl;
        } else {
            std::cerr << "child exited with status " << exit_code << std::endl;
        }
#endif
        checkChildProcesses(false, false);
    };
}

Hub::~Hub()
{
    m_python.reset();

    params.quit();
    settings.quit();
    session.quit();

    stopVrb();
    while (!m_vrbSockets.empty()) {
        auto it = m_vrbSockets.begin();
        auto sock = it->second;
        if (sock) {
            removeSocket(sock);
        }
        m_vrbSockets.erase(it);
    }

    if (!m_isMaster) {
        sendMaster(message::RemoveHub(m_hubId));
        sendMaster(message::CloseConnection("terminating"));
    }

    {
        std::unique_lock<std::mutex> lock(m_socketMutex);
        while (!m_sockets.empty()) {
            auto sock = m_sockets.begin()->first;
            lock.unlock();
            removeSocket(sock);
            lock.lock();
        }
    }

    m_tunnelManager.cleanUp();
    m_dataProxy.reset();

    message::clear_request_queue();

    stopIoThreads();

    hub_instance = nullptr;

    m_config.reset();
}

bool Hub::isPrincipal() const
{
    return m_isMaster;
}

void Hub::initiateQuit()
{
    m_quitting = true;
    vistle::message::prepare_shutdown();
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

// from https://stackoverflow.com/questions/59761124/use-boostprogram-options-to-specify-multiple-flags?cmdf=boost+program_options+verbose+multiple+times

class CountValue: public boost::program_options::typed_value<int> {
public:
    CountValue(): CountValue(nullptr) {}

    CountValue(int *store): boost::program_options::typed_value<int>(store)
    {
        // Ensure that no tokens may be passed as a value.
        default_value(*store);
        zero_tokens();
    }

    virtual ~CountValue() {}

    virtual void xparse(boost::any &store, const std::vector<std::string> & /*tokens*/) const
    {
        // Replace the stored value with the access count.
        store = boost::any(++count_);
    }

private:
    mutable int count_{0};
};

boost::program_options::options_description &Hub::options()
{
    namespace po = boost::program_options;

    static po::options_description desc("usage");
    if (!desc.options().empty()) {
        return desc;
    }

    // clang-format off
    desc.add_options()
        ("help,h", "show this message")
        ("version,V", "print version")
        ("hub,c", po::value<std::string>(), "connect to hub")
        ("batch,b", "do not start user interface")
        ("proxy", "run master hub acting only as a proxy, does not require MPI")
        ("gui,g", "start graphical user interface")
        ("shell,s", "start interactive Python shell (requires ipython or python)")
        ("port,p", po::value<unsigned short>(), "control port")
        ("dataport", po::value<unsigned short>(), "data port")
        ("execute,e", "call compute() after workflow has been loaded")
        ("snapshot", po::value<std::string>(), "store screenshot of workflow to this location")
        ("libsim,l", po::value<std::string>(), "connect to a LibSim instrumented simulation by entering the path to the .sim2 file")
        ("cover", "use OpenCOVER.mpi to manage Vistle session on cluster")
        ("vrb", po::value<std::string>(), "how to launch VRB on principal hub (tui/gui/no)")
        ("exposed,gateway-host,gateway,gw", po::value<std::string>(), "ports are exposed externally on this host")
        ("root", po::value<std::string>(), "path to Vistle build directory")
        ("buildtype", po::value<std::string>(), "build type suffix to binary in Vistle build directory")
        ("conference,conf", po::value<std::string>(), "URL of associated conference call")
        ("url", "Vistle URL, script to process, or slave name")
    ;
    // clang-format on

    return desc;
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

    m_dir = std::make_unique<Directory>(argc, argv);

    m_name = hostname();

    m_config.reset(new config::Access(m_name, m_name));
    m_config->setPrefix(m_dir->prefix());

    if (auto env_logfile = getenv("VISTLE_LOGFILE")) {
        m_logfileFormat = env_logfile;
    } else {
        m_logfileFormat = *m_config->value<std::string>("system", "logfile", platform, "");
        if (m_logfileFormat.empty() && !platform_fallback.empty()) {
            m_logfileFormat = *m_config->value<std::string>("system", "logfile", platform_fallback, "");
        }
    }

    m_basePort = *m_config->value<int64_t>("system", "net", "controlport", m_basePort);

    m_messageBacklog = *m_config->value<int64_t>("system", "hub", "messagebacklog", m_messageBacklog);

    double portDistance = *m_config->value<double>("gui", "module", "port_spacing", 0.);
    double portSize = *m_config->value<double>("gui", "module", "port_size", 0.);
    m_gridSpacingX = portDistance + portSize;
    m_gridSpacingY = m_gridSpacingX;

    namespace po = boost::program_options;
    auto desc = options();
    desc.add_options()("quiet,q", "run quietly")("verbose,v", new CountValue(&m_verbose),
                                                 "increase verbosity (use multiple times for more output)");
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

    if (vm.count("quiet") > 0) {
        m_verbose = Verbosity::Quiet;
    }

    if (vm.count("help")) {
        std::cout << argv[0] << " " << desc << std::endl;
        return false;
    }

    if (vm.count("version")) {
        std::cout << vistle::version::banner() << std::endl;
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
    if (m_verbose >= Verbosity::DuplicateMessages) {
        m_stateTracker.setVerbose(true);
    }
    if (m_verbose >= Verbosity::Messages) {
        m_traceMessages = message::ANY;
    }

    if (vm.count("exposed") > 0) {
        m_exposedHost = vm["exposed"].as<std::string>();
        boost::asio::ip::tcp::resolver resolver(m_ioContext);
        boost::system::error_code ec;
        auto endpoints = resolver.resolve(m_exposedHost, std::to_string(dataPort()),
                                          boost::asio::ip::tcp::resolver::numeric_service, ec);
        if (ec) {
            CERR << "could not resolve gateway host " << m_exposedHost << ": " << ec.message() << std::endl;
            return false;
        } else if (endpoints.begin() != endpoints.end()) {
            auto endpoint = *endpoints.begin();
            m_exposedHostAddr = endpoint.endpoint().address();
            CERR << "AddHub: exposed host " << m_exposedHost << " resolved to " << m_exposedHostAddr << std::endl;
        }
    }

    if (vm.count("root")) {
        std::string buildtype;
        if (vm.count("buildtype"))
            buildtype = vm["buildtype"].as<std::string>();
        m_dir = std::make_unique<Directory>(vm["root"].as<std::string>(), buildtype);
        CERR << "set prefix to " << m_dir->prefix() << std::endl;
    }

    std::string uiCmd = "vistle_gui";
    if (const char *pbs_env = getenv("PBS_ENVIRONMENT")) {
        if (std::string("PBS_INTERACTIVE") != pbs_env) {
            if (!vm.count("batch"))
                CERR << "apparently running in PBS batch mode - defaulting to no UI" << std::endl;
            uiCmd.clear();
        }
    }
    if (vm.count("hub") > 0) {
        uiCmd.clear();
    }
    if (vm.count("vrb")) {
        if (vm["vrb"].as<std::string>() == "tui") {
            m_vrbMode = VrbMode::VrbTui;
        } else if (vm["vrb"].as<std::string>() == "gui") {
            m_vrbMode = VrbMode::VrbGui;
        } else if (vm["vrb"].as<std::string>() == "no") {
            m_vrbMode = VrbMode::VrbNo;
        } else {
            CERR << "invalid VRB mode: " << vm["vrb"].as<std::string>() << ", valid modes are: tui, gui, no"
                 << std::endl;
            return false;
        }
    }
    bool pythonUi = false;
    if (vm.count("shell")) {
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

    if (vm.count("conference")) {
        m_conferenceUrl = vm["conference"].as<std::string>();
    }

    std::string file;
    ConnectionData connectionData;
    if (vm.count("url") == 1) {
        file = vm["url"].as<std::string>();
        Url url(file);
        if (VistleUrl::parse(file, connectionData)) {
            file.clear();
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
                std::string uipath = m_dir->bin() + uiCmd;
                startUi(uipath, true);
            } else if (connectionData.kind == "/open" || connectionData.kind == "/exec" ||
                       connectionData.kind == "/execute") {
                using boost::algorithm::ends_with;
                auto wf = url.fragment();

                file.clear();
                for (vistle::filesystem::path dir(m_dir->prefix()); dir.has_parent_path(); dir = dir.parent_path()) {
                    if (!vistle::filesystem::exists(dir))
                        break;
                    auto exts = {"", ".vsl", ".py"};
                    for (const auto &ext: exts) {
                        auto wf_ext = wf + ext;
                        if (vistle::filesystem::exists(dir / wf_ext)) {
                            file = (dir / wf_ext).string();
                            break;
                        }
                    }
                    dir = dir.parent_path();
                }
                if (file.empty()) {
                    CERR << "could not locate workflow file " << wf << " in " << m_dir->prefix() << " and its parents"
                         << std::endl;
                    return false;
                } else if (connectionData.kind == "/open") {
                    CERR << "opening workflow from " << file << std::endl;
                } else {
                    m_barrierAfterLoad = true;
                    m_executeModules = true;
                    CERR << "executing workflow from " << file << std::endl;
                }
            }
        } else {
            if (url.valid() && url.scheme() == "file") {
                file = url.path();
            }
        }
    }

    if (m_isMaster && vm.count("hub") > 0) {
        m_isMaster = false;
        ++m_basePort;
        m_masterHost = vm["hub"].as<std::string>();

        // parse port from host string, if given
        // possible cases:
        // - hostname
        // - hostname:port
        // - v4address
        // - v4address:port
        // - v6address
        // - [v6address]
        // - [v6address]:port
        auto brace = m_masterHost.find('[');
        if (brace != std::string::npos) {
            // IPv6 address within braces
            if (brace != 0) {
                CERR << "invalid IPv6 address for master host, [ has to be first character: " << m_masterHost
                     << std::endl;
                return false;
            }
            auto endbrace = m_masterHost.find(']', brace);
            if (endbrace == std::string::npos) {
                CERR << "invalid IPv6 address for master host, ] is required" << m_masterHost << std::endl;
                return false;
            }
            auto colon = m_masterHost.find(':', endbrace);
            if (colon != std::string::npos) {
                m_masterPort = boost::lexical_cast<unsigned short>(m_masterHost.substr(colon + 1));
                m_masterHost = m_masterHost.substr(0, colon);
            }
        } else {
            auto colon = m_masterHost.find(':');
            auto colon2 = m_masterHost.find(':', colon + 1);
            if (colon != std::string::npos && colon2 == std::string::npos) {
                m_masterPort = boost::lexical_cast<unsigned short>(m_masterHost.substr(colon + 1));
                m_masterHost = m_masterHost.substr(0, colon);
            } else {
                // IPv6 address without port, not within braces
            }
        }
    }

    if (vm.count("proxy") > 0) {
        if (m_inManager) {
            CERR << "proxy-only mode is not possible when running in manager" << std::endl;
            return false;
        }
        if (!m_isMaster) {
            CERR << "proxy-only mode is only possible for master hub" << std::endl;
            return false;
        }
        m_proxyOnly = true;
        m_localRanks = 0;
    }

    try {
        if (!startServer()) {
            CERR << "failed to initialise control server" << std::endl;
            return false;
        }
    } catch (std::exception &ex) {
        CERR << "failed to initialise control server on port " << m_port << ": " << ex.what() << std::endl;
        return false;
    }

    try {
        if (m_dataPort > 0) {
            m_dataProxy.reset(new DataProxy(m_stateTracker, m_dataPort ? m_dataPort : m_port + 1, !m_dataPort));
        } else {
            m_dataProxy.reset(new DataProxy(m_stateTracker, 0, 0));
        }
        m_dataProxy->setTrace(m_traceMessages);
        if (m_localRanks >= 0)
            m_dataProxy->setNumRanks(m_localRanks);
    } catch (std::exception &ex) {
        CERR << "failed to initialise data server on port " << (m_dataPort ? m_dataPort : m_port + 1) << ": "
             << ex.what() << std::endl;
        return false;
    }

    {
        std::stringstream s;
        s << hostname() << " " << port() << " " << dataPort() << " " << dataManagerBase();
        std::string conn = s.str();
        setenv("VISTLE_CONNECTION", conn.c_str(), 1);
    }

    vistle::directory::setVistleRoot(m_dir->prefix(), m_dir->buildType());

    if (m_isMaster) {
        // this is the master hub
        m_hubId = Id::MasterHub;
        params.setId(m_hubId);
        m_stateTracker.setId(m_hubId);
        if (!m_inManager) {
            message::DefaultSender::init(m_hubId, 0);
        }
        make.setId(m_hubId);
        make.setRank(0);
        Router::init(message::Identify::HUB, m_hubId);

        m_masterPort = m_port;
        m_dataProxy->setHubId(m_hubId);

        m_scriptPath = file;
    } else {
        if (!file.empty())
            m_name = file;

        if (!connectToMaster(m_masterHost, m_masterPort)) {
            CERR << "failed to connect to master at " << m_masterHost << ":" << m_masterPort << std::endl;
            return false;
        }
    }

    if (vm.count("execute") > 0) {
        m_barrierAfterLoad = true;
        m_executeModules = true;
    }

    if (vm.count("snapshot") > 0) {
        m_snapshotFile = vm["snapshot"].as<std::string>();
        m_barrierAfterLoad = true;
    }

    if (!m_inManager && !m_interrupt && !m_quitting) {
        // start UI
        if (!uiCmd.empty()) {
            m_hasUi = true;
            std::string uipath = m_dir->bin() + uiCmd;
            startUi(uipath);
        }
        if (pythonUi) {
            m_hasUi = true;
            startPythonUi();
        }

        if (!m_proxyOnly) {
            std::string libsimArg = vm.count("libsim") > 0 ? vm["libsim"].as<std::string>() : "";
            if (!startManager(vm.count("cover") > 0, libsimArg)) {
                CERR << "FATAL: could not launch cluster manager" << std::endl;
                exit(1);
            }
        }
    }

    if (!m_interrupt && !m_quitting) {
#ifdef HAVE_PYTHON
        m_python.reset(new PythonInterpreter(m_dir->share()));
#endif
    }

    if (m_isMaster && !m_inManager && !m_interrupt && !m_quitting) {
        m_hasVrb = startVrb();
    }

    if (m_proxyOnly) {
        auto master = addHubForSelf();
        if (m_verbose >= Verbosity::Normal) {
            CERR << "principal hub: " << master << std::endl;
        }
        m_stateTracker.handle(master, nullptr);
        sendUi(master);

        if (!hubReady()) {
            m_uiManager.lockUi(false);
            return false;
        }
        m_uiManager.lockUi(false);
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

unsigned short Hub::dataManagerBase() const
{
    auto p = dataPort();
    if (p == 0)
        p = port();
    return p + 1;
}

std::shared_ptr<process::child>
Hub::launchProcess(int type, const std::string &prog, const std::vector<std::string> &args, std::string name,
                   std::function<bool(std::shared_ptr<process::child>, std::shared_ptr<process::ipstream>)> parseOutput)
{
    if (name.empty()) {
        name = prog;
    }
    {
        auto pos = name.find_last_of('/');
        if (pos != std::string::npos) {
            name = name.substr(pos + 1);
        }
    }

    auto path = process::search_path(prog);
    if (path.empty()) {
        boost::system::error_code ec;
        if (filesystem::exists(prog, ec)) {
            path = prog;
#ifdef _WIN32
        } else if (filesystem::exists(prog + ".exe")) {
            path = prog + ".exe";
        } else if (filesystem::exists(prog + ".bat")) {
            path = prog + ".bat";
#endif
        } else {
            std::stringstream info;
            info << "Cannot launch " << prog << ": not found";
            sendError(info.str());
            return nullptr;
        }
    }

    std::string typePrefix;
    switch (type) {
    case Process::Manager:
        typePrefix = "Mgr";
        break;
    case Process::Cleaner:
        typePrefix = "Cln";
        break;
    case Process::GUI:
        typePrefix = "UI";
        break;
    case Process::VRB:
        typePrefix = "VRB";
        break;
    default:
        typePrefix = "MOD";
        break;
    }

    int moduleId = message::Id::isModule(type) ? type : message::Id::Invalid;
    std::string logfile;
    process::environment env = boost::this_process::environment();
    process::environment childEnv = env;
#ifdef VISTLE_USE_FMT
    const auto *user = getenv("USER");
    if (!user) {
        user = getenv("USERNAME");
    }
    if (!user) {
        user = "unknown";
    }
    const auto *tmpdir = getenv("TMPDIR");
    if (!tmpdir) {
        tmpdir = getenv("TEMP");
    }
    if (!tmpdir) {
        tmpdir = "/tmp";
    }
    fmt::dynamic_format_arg_store<fmt::format_context> nargs;
    nargs.push_back(fmt::arg("name", name));
    nargs.push_back(fmt::arg("prog", prog));
    nargs.push_back(fmt::arg("type", typePrefix));
    nargs.push_back(fmt::arg("hub", m_hubId));
    nargs.push_back(fmt::arg("port", m_port));
    nargs.push_back(fmt::arg("host", hostname()));
    nargs.push_back(fmt::arg("user", user));
    nargs.push_back(fmt::arg("uid", getuid()));
    nargs.push_back(fmt::arg("id", moduleId));
    nargs.push_back(fmt::arg("tmpdir", tmpdir));
    if (!m_logfileFormat.empty()) {
        logfile = fmt::vformat(m_logfileFormat, nargs);
    }
#else
    auto bopen = m_logfileFormat.find("{");
    auto bclose = m_logfileFormat.find("}");
    if (bopen == std::string::npos && bclose == std::string::npos) {
        logfile = m_logfileFormat;
    } else {
        std::stringstream info;
        info << "fmt library not available, cannot format logfile name: " << m_logfileFormat << std::endl;
        sendInfo(info.str());
        logfile.clear();
    }
#endif
    if (logfile.empty()) {
        if (childEnv.find("VISTLE_LOGFILE") != childEnv.end()) {
            childEnv["VISTLE_LOGFILE"].clear();
        }
        if (m_verbose >= Verbosity::ManagerMessages) {
            CERR << "logfile format is empty, not setting VISTLE_LOGFILE environment variable" << std::endl;
        }
    } else {
        childEnv["VISTLE_LOGFILE"] = logfile;
        if (m_verbose >= Verbosity::Manager) {
            CERR << "setting VISTLE_LOGFILE to " << logfile << std::endl;
        }
    }

    std::shared_ptr<process::child> child;
    auto out = std::make_shared<process::ipstream>();
    auto err = std::make_shared<process::ipstream>();
    try {
        child = std::make_shared<process::child>(
            path, process::args(args), terminate_with_parent(), process::environment(childEnv), process::std_out > *out,
            process::std_err > *err, process::std_in < process::null, m_ioContext, process::on_exit(exit_handler));
    } catch (std::exception &ex) {
        std::stringstream info;
        info << "Failed to launch: " << path << ": " << ex.what();
        sendError(info.str());
        return nullptr;
    }

    if (parseOutput) {
        if (!parseOutput(child, out)) {
            return nullptr;
        }
    }


    auto consumeStream = [this, type, typePrefix, child](process::ipstream &str, message::SendText::TextType stream,
                                                         ObservedChild &obs, std::deque<ObservedChild::TaggedLine> &buf,
                                                         size_t &discardCount, std::mutex &mutex) mutable {
        std::string line;
        while (child->running() && getline_multi_delim(str, line, "\n\r")) {
            std::string prefix;
            bool print = false;
            switch (type) {
            case Process::Manager:
            case Process::Cleaner:
            case Process::GUI:
            case Process::VRB:
                prefix = "[" + typePrefix + "] ";
                if (m_verbose >= Verbosity::Manager) {
                    print = true;
                }
                break;
            default:
                prefix = "[" + std::to_string(obs.moduleId) + "] ";
                if (m_verbose >= Verbosity::Modules) {
                    print = true;
                }
                break;
            }
            if (print) {
                if (stream == message::SendText::Cout)
                    std::cout << prefix + line << std::flush;
                else
                    std::cerr << prefix + line << std::flush;
            }
            std::lock_guard<std::mutex> lock(mutex);
            if (obs.streamOutput) {
                obs.sendTextToUi(stream, discardCount + buf.size(), line, obs.moduleId);
            }
            buf.emplace_back(stream, line);
            while (m_messageBacklog >= 0 && buf.size() > size_t(m_messageBacklog)) {
                ++discardCount;
                buf.pop_front();
            }
        }
    };

    auto &obs = m_observedChildren[child->id()];
    obs.hub = this;
    obs.child = child;
    obs.name = name;
    obs.childId = child->id();
    obs.moduleId = moduleId;
    obs.outThread = std::make_unique<std::thread>([consumeStream, out, &obs]() mutable {
        setThreadName("cout:" + obs.name);
        consumeStream(*out, message::SendText::Cout, obs, obs.buffer, obs.numDiscarded, obs.mutex);
    });
    obs.errThread = std::make_unique<std::thread>([consumeStream, err, &obs]() mutable {
        setThreadName("cerr:" + obs.name);
        consumeStream(*err, message::SendText::Cerr, obs, obs.buffer, obs.numDiscarded, obs.mutex);
    });

    if (!child || !child->valid()) {
        return nullptr;
    }

    std::lock_guard<std::mutex> guard(m_processMutex);
    m_processMap[child] = type;
    return child;
}

std::shared_ptr<process::child> Hub::launchMpiProcess(int type, const std::vector<std::string> &args)
{
    assert(!m_proxyOnly);
    assert(!args.empty());
    auto prog = args[0];
#ifdef VISTLE_USE_MPI
    std::string spawn_cmd = "";
    std::string spawn = *m_config->value<std::string>("system", "mpirun", platform, spawn_cmd);
    if (spawn.empty() && !platform_fallback.empty()) {
        spawn = *m_config->value<std::string>("system", "mpirun", platform_fallback, spawn_cmd);
    }
    if (m_verbose >= Verbosity::Modules) {
        CERR << "launchMpiProcess: " << spawn << " for " << prog << std::endl;
    }

    auto child = launchProcess(type, spawn, args, prog);
#ifndef _WIN32
    if (!child && spawn != "mpirun") {
        CERR << "launchMpiProcess: failed to execute " << args[0] << " via " << spawn << ", retrying with mpirun"
             << std::endl;
        auto child = launchProcess(type, spawn, args, prog);
    }
#endif

#else // VISTLE_USE_MPI
    std::vector<std::string> nargs(args.begin() + 1, args.end());
    auto child = launchProcess(type, prog, nargs, prog);
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

    std::unique_ptr<std::mutex> closemutex;
    std::unique_ptr<std::condition_variable> closecv;
    bool closed = false;
    if (close) {
        closemutex = std::make_unique<std::mutex>();
        closecv = std::make_unique<std::condition_variable>();
    }

    message::async_send(*sock, msg, pl,
                        [this, sock, close, &closemutex, &closecv, &closed](boost::system::error_code ec) {
                            if (close) {
                                std::unique_lock lk(*closemutex);
                                closecv->wait(lk, [] { return true; });
                                closed = true;
                                lk.unlock();
                                closecv->notify_one();
                            } else if (ec) {
                                if (ec.value() == boost::asio::error::eof) {
                                    if (!m_quitting)
                                        CERR << "send error, socket closed: " << ec.message() << std::endl;
                                    removeSocket(sock);
                                } else if (ec.value() == boost::asio::error::broken_pipe) {
                                    if (!m_quitting)
                                        CERR << "send error, broken pipe: " << ec.message() << std::endl;
                                    removeSocket(sock);
#if 0
                                } else if (ec.code() == boost::system::errc::broken_pipe) {
                                    if (!m_quitting)
                                        CERR << "send error, socket closed: " << ec.message() << std::endl;
                                    removeSocket(sock);
#endif
                                } else {
                                    if (!m_quitting)
                                        CERR << "send error: " << ec.message() << std::endl;
                                }
                            }
                        });
#endif
    if (close) {
        closecv->notify_one();
        std::unique_lock lk(*closemutex);
        closecv->wait(lk, [&closed] { return closed; });
        removeSocket(sock);
        return true;
    }
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
        cd.conference_url = m_conferenceUrl;
        setSessionUrl(VistleUrl::create(cd));

        if (m_verbose >= Verbosity::Normal) {
            CERR << "listening for connections on port " << m_port << std::endl;
        }
    }

    startAccept(m_acceptorv4);
    startAccept(m_acceptorv6);

    return true;
}

bool Hub::startAccept(std::shared_ptr<acceptor> a)
{
    if (!a->is_open())
        return false;

    socket_ptr sock(new asio::ip::tcp::socket(m_ioContext));
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
    std::unique_lock<std::mutex> lock(m_socketMutex);
    bool ok = m_sockets.insert(std::make_pair(sock, ident)).second;
    assert(ok);
    (void)ok;
}

bool Hub::removeSocket(Hub::socket_ptr sock, bool close)
{
    if (!sock) {
        return false;
    }

    if (close) {
        bool open = sock->is_open();
        boost::system::error_code ec;
        sock->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        if (ec && ec != boost::system::errc::not_connected && open) {
            CERR << "socket shutdown failed: " << ec.message() << std::endl;
        }
        ec.clear();
        sock->close(ec);
        if (ec && open) {
            CERR << "closing socket failed: " << ec.message() << std::endl;
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
                if (m_verbose >= Verbosity::Manager) {
                    CERR << "lost connection to VRB for module " << s.first << std::endl;
                }
                s.second.reset();
                break;
            }
        }
    } else {
        if (m_masterSocket == sock) {
            CERR << "lost connection to master" << std::endl;
            m_masterSocket.reset();
        }
    }

    std::unique_lock<std::mutex> lock(m_socketMutex);
    if (removeClient(sock)) {
        //CERR << "removed client" << std::endl;
    }

    bool ret = m_sockets.erase(sock) > 0;
    sock.reset();
    return ret;
}

void Hub::addClient(Hub::socket_ptr sock)
{
    std::unique_lock<std::mutex> lock(m_socketMutex);
    //CERR << "new client" << std::endl;
#ifdef BOOST_POSIX_API
    fcntl(sock->native_handle(), F_SETFD, FD_CLOEXEC);
#endif
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

    startIoThread();

    auto state = m_stateTracker.getLockedState();
    for (auto &m: state.messages) {
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
        avail = sizeof(uint32_t) - 1;
        std::unique_lock<std::mutex> lock(m_socketMutex);
        for (auto &s: m_clients) {
            if (!s) {
                continue;
            }
            if (!s->is_open()) {
                lock.unlock();
                CERR << "socket closed" << std::endl;
                removeSocket(s);
                return true;
            }
            boost::asio::socket_base::bytes_readable command(true);
            try {
                s->io_control(command);
            } catch (std::exception &ex) {
                lock.unlock();
                CERR << "socket error: " << ex.what() << std::endl;
                removeSocket(s);
                return true;
            }
            if (command.get() > avail) {
                avail = command.get();
                sock = s;
            }
        }
        lock.unlock();
        if (sock) {
            work = true;
            handleWrite(sock);
        }
    } while (!m_interrupt && !m_quitting && avail >= sizeof(uint32_t));

    if (m_interrupt) {
        if (m_isMaster) {
            auto quit = make.message<message::Quit>();
            sendSlaves(quit);
            sendSlaves(make.message<message::CloseConnection>("user interrupt"));
            handleMessage(quit);
        }
        emergencyQuit();
    }

    if (!m_isMaster && !m_masterSocket) {
        emergencyQuit();
    }

    if (m_quitting) {
        if (checkChildProcesses())
            work = true;

        if (!hasChildProcesses(true)) {
            ret = false;
        } else {
            if (m_verbose >= Verbosity::Manager) {
                std::lock_guard<std::mutex> guard(m_processMutex);
                if (m_prevNumRunning != m_processMap.size()) {
                    m_prevNumRunning = m_processMap.size();
                    CERR << "still " << m_processMap.size() << " processes running" << std::endl;
                    for (const auto &process: m_processMap) {
                        auto pid = process.first->id();
                        std::cerr << "   id: " << process.second << ", pid: " << pid;
                        auto it = m_observedChildren.find(pid);
                        if (it != m_observedChildren.end()) {
                            std::cerr << ", " << it->second.name;
                        }
                        std::cerr << std::endl;
                    }
                    m_lastProcessCheck = std::chrono::steady_clock::now();
                } else if (std::chrono::steady_clock::now() - m_lastProcessCheck > std::chrono::seconds(1)) {
                    std::cerr << "." << std::flush;
                    m_lastProcessCheck = std::chrono::steady_clock::now();
                }
            }
        }

        if (m_dataProxy)
            m_dataProxy->cleanUp();
        m_tunnelManager.cleanUp();
    }

    if (checkOutstandingDataConnections()) {
        work = true;
    }


    vistle::adaptive_wait(work);

    if (ret == false) {
        if (m_verbose >= Verbosity::Manager) {
            CERR << "dispatch: returning false for Quit" << std::endl;
        }
    }

    return ret;
}

bool Hub::checkOutstandingDataConnections()
{
    bool changed = false;

    std::unique_lock<std::mutex> dataConnGuard(m_outstandingDataConnectionMutex);
    for (auto it = m_outstandingDataConnections.begin(); it != m_outstandingDataConnections.end(); ++it) {
        if (!it->second.fut.valid())
            continue;
        if (it->second.fut.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            continue;

        changed = true;

        auto &add = it->first;
        bool ok = it->second.fut.get();
        if (ok) {
            m_stateTracker.handle(add, it->second.payload.get(), true);
            sendUi(add, message::Id::Broadcast, it->second.payload.get());
        } else {
            CERR << "could not establish data connection to hub " << add.id() << " at " << add.address() << ":"
                 << add.port() << std::endl;
            if (m_isMaster) {
                CERR << "removing hub " << add.id() << " at " << add.address() << ":" << add.port() << std::endl;
                auto rm = make.message<message::RemoveHub>(add.id());
                m_stateTracker.handle(rm, nullptr);
                sendSlaves(rm);
                sendManager(rm);
                removeSlave(add.id());
            } else if (add.id() == Id::MasterHub) {
                CERR << "terminating: cannot continue without master data connection" << std::endl;
                emergencyQuit();
            }
        }
        m_outstandingDataConnections.erase(it);
        break;
    }

    return changed;
}

void Hub::handleWrite(Hub::socket_ptr sock)
{
    message::Identify::Identity senderType = message::Identify::UNKNOWN;
    {
        std::unique_lock<std::mutex> lock(m_socketMutex);
        auto it = m_sockets.find(sock);
        if (it != m_sockets.end())
            senderType = it->second;
    }

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
            initiateQuit();
            //break;
        }
    } else if (ec) {
        CERR << "error during read from socket: " << ec.message() << std::endl;
        removeSocket(sock);
        //initiateQuit();
    }
}

bool Hub::sendMaster(const message::Message &msg, const buffer *payload)
{
    assert(!m_isMaster);
    if (m_isMaster) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_socketMutex);
    int numSent = 0;
    for (auto &sock: m_sockets) {
        if (sock.second == message::Identify::HUB) {
            auto s = sock.first;
            lock.unlock();
            bool close = msg.type() == message::CLOSECONNECTION;
            if (!sendMessage(s, msg, payload) && !close) {
                removeSocket(s);
                break;
            }
            ++numSent;
            if (close) {
                break;
            }
        }
    }
    assert(numSent <= 1);
    return numSent == 1;
}

bool Hub::sendManager(const message::Message &msg, int hub, const buffer *payload)
{
    if (hub == Id::LocalHub || hub == m_hubId || (hub == Id::MasterHub && m_isMaster)) {
        assert(m_managerConnected || m_proxyOnly);
        if (m_proxyOnly) {
            return true;
        }
        if (!m_managerConnected) {
            CERR << "sendManager: no connection, cannot send " << msg << std::endl;
            return false;
        }

        int numSent = 0;
        std::unique_lock<std::mutex> lock(m_socketMutex);
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
        CERR << "sendModule: id " << id << " is not for a module: " << msg << std::endl;
        return false;
    }

    int hub = idToHub(id);
    if (Id::Invalid == hub) {
        CERR << "sendModule: could not find hub for id " << id << ": " << msg << std::endl;
        return false;
    }

    if (hub == m_hubId)
        return sendManager(msg, hub, payload);

    return sendHub(hub, msg, payload);
}

bool Hub::sendAll(const message::Message &msg, const buffer *payload)
{
    if (m_isMaster) {
        sendUi(msg, Id::Broadcast, payload);
        sendManager(msg, Id::LocalHub, payload);
        sendSlaves(msg, true /* return to sender */, payload);
    } else {
        return sendMaster(msg, payload);
    }

    return true;
}

bool Hub::sendAllUi(const message::Message &msg, const buffer *payload)
{
    if (m_isMaster) {
        sendUi(msg, Id::Broadcast, payload);
    } else {
        return sendMaster(msg, payload);
    }
    return true;
}

bool Hub::sendAllButUi(const message::Message &msg, const buffer *payload)
{
    if (m_isMaster) {
        sendManager(msg, Id::LocalHub, payload);
        sendSlaves(msg, true /* return to sender */, payload);
    } else {
        return sendMaster(msg, payload);
    }
    return true;
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

Hub::Verbosity Hub::verbosity() const
{
    if (m_verbose < 0)
        return Verbosity::Quiet;
    if (m_verbose >= static_cast<int>(Verbosity::AllMessages))
        return Verbosity::AllMessages;
    return static_cast<Verbosity>(m_verbose);
}

message::AddHub Hub::addHubForSelf() const
{
    auto hub = make.message<message::AddHub>(m_hubId, m_name);
    hub.setNumRanks(m_localRanks);
    //hub.setDestId(Id::ForBroadcast);
    hub.setPort(m_port);
    hub.setDataPort(m_dataProxy->port());
    if (m_dataProxy->port() > 0) {
        hub.setDataManagerPort(m_dataProxy->port() + 1);
    } else {
        hub.setDataManagerPort(m_port + 1);
    }
    hub.setLoginName(vistle::getLoginName());
    hub.setRealName(vistle::getRealName());
    hub.setHasUserInterface(m_hasUi);
    hub.setHasVrb(m_hasVrb);

    if (!m_exposedHost.empty()) {
        hub.setAddress(m_exposedHostAddr);
        //CERR << "AddHub: exposed host: " << m_exposedHostAddr << std::endl;
    }

    hub.setSystemType(vistle::version::os());
    hub.setArch(vistle::version::arch());
    hub.setVersion(vistle::version::string());
    hub.setInfo(vistle::version::flags());

    return hub;
}

bool Hub::hubReady()
{
    assert(m_proxyOnly || m_managerConnected);
    if (m_isMaster) {
        m_ready = true;

        CompressionSettings cs;

        auto archiveCompression = session.addIntParameter("archive_compression", "compression mode for archives",
                                                          message::CompressionNone, Parameter::Choice);
        session.V_ENUM_SET_CHOICES(archiveCompression, message::CompressionMode);
        auto archiveCompressionSpeed =
            session.addIntParameter("archive_compression_speed", "speed parameter of compression algorithm", -1);

        session.setParameterRange(archiveCompressionSpeed, Integer(-1), Integer(100));

        auto compressionMode = session.addIntParameter(CompressionSettings::p_mode, "compression mode for data fields",
                                                       cs.mode, Parameter::Choice);
        session.V_ENUM_SET_CHOICES(compressionMode, FieldCompressionMode);

        session.setCurrentParameterGroup("zfp");
        auto zfpMode = session.addIntParameter(CompressionSettings::p_zfpMode, "mode for zfp compression", cs.zfpMode,
                                               Parameter::Choice);
        session.V_ENUM_SET_CHOICES(zfpMode, FieldCompressionZfpMode);
        auto zfpRate = session.addFloatParameter(CompressionSettings::p_zfpRate,
                                                 "zfp fixed compression rate (bits/value)", cs.zfpRate);
        session.setParameterRange(zfpRate, Float(1), Float(64));
        auto zfpPrecision = session.addIntParameter(CompressionSettings::p_zfpPrecision,
                                                    "zfp fixed precision (no. bit planes)", cs.zfpPrecision);
        session.setParameterRange(zfpPrecision, Integer(1), Integer(64));
        auto zfpAccuracy = session.addFloatParameter(CompressionSettings::p_zfpAccuracy, "zfp absolute error tolerance",
                                                     cs.zfpAccuracy);
        session.setParameterRange(zfpAccuracy, Float(0.), Float(1e10));

        session.setCurrentParameterGroup("SZ");
        session.V_ENUM_SET_CHOICES(compressionMode, FieldCompressionMode);
        auto szAlgo = session.addIntParameter(CompressionSettings::p_szAlgo, "SZ3 compression algorithm", cs.szAlgo,
                                              Parameter::Choice);
        session.V_ENUM_SET_CHOICES(szAlgo, FieldCompressionSzAlgo);
        auto szError = session.addIntParameter(CompressionSettings::p_szError, "SZ3 error control method", cs.szError,
                                               Parameter::Choice);
        session.V_ENUM_SET_CHOICES(szError, FieldCompressionSzError);
        auto szRel =
            session.addFloatParameter(CompressionSettings::p_szRelError, "SZ3 relative error tolerance", cs.szRelError);
        session.setParameterRange(szRel, Float(0.), Float(1.));
        auto szAbs =
            session.addFloatParameter(CompressionSettings::p_szAbsError, "SZ3 absolute error tolerance", cs.szAbsError);
        session.setParameterMinimum(szAbs, Float(0.));
        auto szPsnr =
            session.addFloatParameter(CompressionSettings::p_szPsnrError, "SZ3 PSNR tolerance", cs.szPsnrError);
        session.setParameterMinimum(szPsnr, Float(0.));
        auto szL2 =
            session.addFloatParameter(CompressionSettings::p_szL2Error, "SZ3 L2 norm error tolerance", cs.szL2Error);
        session.setParameterMinimum(szL2, Float(0.));

        session.setCurrentParameterGroup("BigWhoop");
        auto bwNPar = session.addIntParameter(CompressionSettings::p_bigWhoopNPar,
                                              "BigWhoop number of independent parameters", cs.bigWhoopNPar);
        session.setParameterRange(bwNPar, Integer(1), Integer(std::numeric_limits<uint8_t>::max()));

        auto bwRate = session.addFloatParameter(CompressionSettings::p_bigWhoopRate, "BigWhoop compression rate",
                                                std::stof(cs.bigWhoopRate));
        session.setParameterMinimum(bwRate, Float(0.));

        session.setCurrentParameterGroup("");

        for (auto s: m_slavesToConnect) {
            auto set = make.message<message::SetId>(s->id);
            sendMessage(s->sock, set);
        }
        m_slavesToConnect.clear();

        if (!processStartupScripts() || !processScript()) {
            initiateQuit();
            auto q = make.message<message::Quit>();
            sendSlaves(q);
            sendManager(q);
            handleMessage(q);
            return false;
        }

        if (!m_quitting && !m_snapshotFile.empty()) {
            std::cerr << "requesting screenshot to " << m_snapshotFile << std::endl;
            auto msg = make.message<message::Screenshot>(m_snapshotFile, true);
            msg.setDestId(message::Id::Broadcast);
            sendUi(msg);
            return true;
        }
    } else {
        auto hub = addHubForSelf();
        std::unique_lock<std::mutex> lock(m_socketMutex);
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
                }
            }
        }
        lock.unlock();

        auto pl = message::addPayload(hub, m_addHubPayload);
        m_stateTracker.handle(hub, &pl, true);

        if (!sendMaster(hub, &pl)) {
            return false;
        }
        m_ready = true;
        return processStartupScripts();
    }
    return true;
}

bool Hub::handleMessage(const message::Message &recv, Hub::socket_ptr sock, const buffer *payload,
                        message::Identify::Identity senderType)
{
    using namespace vistle::message;

    checkLastModuleQuit();

    message::Buffer buf(recv);
    Message &msg = buf;
    {
        std::unique_lock<std::mutex> lock(m_socketMutex);
        auto it = m_sockets.find(sock);
        if (sock) {
            assert(it != m_sockets.end());
            if (it != m_sockets.end())
                senderType = it->second;
        }
    }

    if (senderType == Identify::UI)
        msg.setSenderId(m_hubId);

    try {
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
            CERR << "remote closes socket - reason: " << mm.reason() << std::endl;
            removeSocket(sock);
            if (senderType == Identify::HUB || senderType == Identify::MANAGER) {
                CERR << "terminating." << std::endl;
                emergencyQuit();
                initiateQuit();
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
            if (m_verbose >= Verbosity::Manager) {
                CERR << "ident msg: " << id.identity() << std::endl;
            }
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
                std::unique_lock<std::mutex> lock(m_socketMutex);
                auto it = m_sockets.find(sock);
                assert(it != m_sockets.end());
                if (it != m_sockets.end())
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
                m_localManagerRank0Pid = id.pid();
                m_dataProxy->setNumRanks(id.numRanks());
                m_dataProxy->setBoostArchiveVersion(id.boost_archive_version());
                m_dataProxy->setIndexSize(id.indexSize());
                m_dataProxy->setScalarSize(id.scalarSize());
                m_addHubPayload = getPayload<AddHub::Payload>(*payload);
                if (m_verbose >= Verbosity::Normal) {
                    CERR << "manager connected with " << m_localRanks << " ranks" << std::endl;
                }

                if (m_verbose >= Verbosity::ManagerMessages) {
                    sendMessage(sock, make.message<message::Trace>(message::Id::Broadcast, message::ANY, true));
                }

                if (m_hubId == Id::MasterHub) {
                    auto master = addHubForSelf();
                    master.setDestId(Id::Broadcast);
                    if (m_verbose >= Verbosity::Manager) {
                        CERR << "principal hub: " << master << std::endl;
                    }
                    auto pl = message::addPayload(master, m_addHubPayload);
                    m_stateTracker.handle(master, &pl);
                    sendUi(master, message::Id::Broadcast, &pl);
                }

                if (m_hubId != Id::Invalid) {
                    auto set = make.message<message::SetId>(m_hubId);
                    sendMessage(sock, set);
                    if (message::Id::isHub(m_hubId)) {
                        auto state = m_stateTracker.getLockedState();
                        for (auto &m: state.messages) {
                            sendMessage(sock, m.message, m.payload.get());
                        }
                    }
                    if (!hubReady()) {
                        m_uiManager.lockUi(false);
                        return false;
                    }
                }
                m_uiManager.lockUi(false);
                if (!m_snapshotFile.empty()) {
                    //handleMessage(make.message<Quit>());
                }
                break;
            }
            case Identify::UI: {
                m_uiManager.addClient(sock);
                break;
            }
            case Identify::HUB: {
                if (m_isMaster) {
                    CERR << "refusing connection from other master hub" << std::endl;
                    auto close = make.message<CloseConnection>("refusing connection from other master hub");
                    sendMessage(sock, close);
                    return true;
                }
                assert(!m_isMaster);
                CERR << "master hub connected" << std::endl;
                break;
            }
            case Identify::SLAVEHUB: {
                if (!m_isMaster) {
                    CERR << "refusing connection from other slave hub, connect directly to a master" << std::endl;
                    auto close = make.message<CloseConnection>("refusing connection from other slave hub");
                    sendMessage(sock, close);
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
                removeSocket(sock, false);
                break;
            }
            case Identify::TUNNEL: {
                m_tunnelManager.addSocket(id, sock);
                removeSocket(sock, false);
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
            auto addPl = getPayload<AddHub::Payload>(*payload);
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
            std::unique_lock<std::mutex> guard(m_outstandingDataConnectionMutex);
            auto it = m_outstandingDataConnections.find(add);
            if (it == m_outstandingDataConnections.end()) {
                m_outstandingDataConnections[add].fut = std::async(
                    std::launch::async, [this, add, addPl]() { return m_dataProxy->connectRemoteData(add, addPl); });
                if (payload) {
                    m_outstandingDataConnections[add].payload = std::make_shared<buffer>(*payload);
                }
            } else {
                CERR << "already connecting to hub " << add.id() << ":" << add << std::endl;
            }
            guard.unlock();

            auto pl = addPayload<AddHub::Payload>(add, addPl);
            m_stateTracker.handle(add, &pl, true);
            sendManager(add, Id::LocalHub, &pl);
            sendUi(add, message::Id::Broadcast, &pl);
            if (m_isMaster) {
                sendSlaves(add, true, &pl);
            }
            break;
        }
        case message::CONNECT: {
            auto &mm = static_cast<const message::Connect &>(msg);
            return handleConnectOrDisconnect(mm);
        }
        case message::DISCONNECT: {
            auto &mm = static_cast<const message::Disconnect &>(msg);
            return handleConnectOrDisconnect(mm);
        }
        case MODULEEXIT: {
            auto &exit = msg.as<ModuleExit>();
            handlePriv(exit);
            break;
        }
        case KILL: {
            auto &kill = msg.as<Kill>();
            handlePriv(kill);
            break;
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
                CERR << "failed to translate module id " << msg.destId() << " to hub: " << msg << std::endl;
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

            if (m_traceMessages == message::ANY || msg.type() == m_traceMessages ||
                (msg.type() == message::SPAWN && m_verbose >= Verbosity::Manager)
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

        checkLastModuleQuit();

        if (Router::the().toHandler(msg, senderType)) {
            switch (msg.type()) {
            case message::CREATEMODULECOMPOUND: {
                if (!payload || payload->empty()) {
                    std::cerr << "missing payload for CREATEMODULECOMPOUND message!" << std::endl;
                    break;
                }
                ModuleCompound comp(msg.as<message::CreateModuleCompound>(), *payload);
#ifdef HAVE_PYTHON
                moduleCompoundToFile(comp);
#endif
                comp.send(std::bind(&Hub::sendManager, this, std::placeholders::_1, message::Id::LocalHub,
                                    std::placeholders::_2));
                break;
            }

            case message::LOADWORKFLOW: {
                auto &load = msg.as<LoadWorkflow>();
                handlePriv(load);
                break;
            }

            case message::SAVEWORKFLOW: {
                auto &save = msg.as<SaveWorkflow>();
                handlePriv(save);
                break;
            }

            case message::SPAWN: {
                auto &spawn = static_cast<const Spawn &>(msg);
                handlePriv(spawn);
                break;
            }

            case message::SETNAME: {
                auto &setname = static_cast<const SetName &>(msg);
                handlePriv(setname);
                break;
            }

            case message::ADDHUB: {
                auto &add = static_cast<const AddHub &>(msg);
                if (m_isMaster && add.hasUserInterface()) {
                    std::map<int, std::string> toMirror;
                    std::map<int, int> blueprintIds;
                    for (const auto &it: m_stateTracker.runningMap) {
                        const auto &m = it.second;
                        if (m.mirrorOfId == m.id) {
                            toMirror[m.id] = m.name;
                        }
                    }

                    for (auto &m: toMirror) {
                        spawnMirror(add.id(), m.second, m.first, blueprintIds[m.first]);
                    }
                }
                break;
            }

            case message::ADDPARAMETER: {
                auto addParam = msg.as<message::AddParameter>();
                if (!m_isMaster) {
                    // set up parameter connections on master hub
                    break;
                }
                if (addParam.destId() == settings.id()) {
                    settings.handleMessage(addParam);
                }
                int id = addParam.senderId();
                if (m_stateTracker.getMirrorId(id) != id) {
                    // connect parameters of mirrored instances to original parameter only
                    handleQueue();
                    break;
                }
                auto pn = addParam.getName();
                auto mirrors = m_stateTracker.getMirrors(id);
                for (auto &m: mirrors) {
                    if (m == id) {
                        // skip connection to self
                        continue;
                    }
                    auto con = message::Connect(id, pn, m, pn);
                    handleMessage(con);
                }

                break;
            }

            case message::REMOVEPARAMETER: {
                if (!m_isMaster) {
                    // set up parameter connections on master hub
                    break;
                }

                auto removeParam = msg.as<message::RemoveParameter>();
                if (removeParam.destId() == session.id()) {
                    session.handleMessage(removeParam);
                }
                if (removeParam.destId() == settings.id()) {
                    settings.handleMessage(removeParam);
                }
                if (removeParam.destId() == params.id()) {
                    params.handleMessage(removeParam);
                }
                break;
            }

            case message::SETPARAMETER: {
                auto setParam = msg.as<message::SetParameter>();

                if (setParam.destId() == session.id()) {
                    session.handleMessage(setParam);
                }

                if (setParam.destId() == settings.id()) {
                    settings.handleMessage(setParam);
                }

                if (setParam.destId() == params.id()) {
                    params.handleMessage(setParam);
                }

                if (Id::isModule(setParam.destId())) {
                    const std::string name = setParam.getName();
                    if (name == "_position" || name == "_layer") {
                        // for old workflows
                        int id = setParam.destId();
                        std::string suffix = "[" + std::to_string(id) + "]";
                        std::string nname = name.substr(1) + suffix;
                        auto nparam = setParam;
                        nparam.setName(nname);
                        nparam.setDestId(Id::Vistle);
                        session.handleMessage(nparam);
                    }
                }

                updateLinkedParameters(setParam);
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

                    bool isCover = spawn.getName() == std::string("COVER");
                    if (m_coverIsManager && isCover) {
                        CERR << "assuming that COVER running cluster manager loads Vistle plugin" << std::endl;
                    } else {
                        if (const AvailableModule *mod = findModule({spawn.hubId(), spawn.getName()})) {
                            spawnModule(mod->path(), spawn.getName(), spawn.spawnId());
                        } else {
                            if (spawn.hubId() == m_hubId) {
                                std::stringstream str;
                                str << "refusing to spawn " << spawn.getName() << ":" << spawn.spawnId()
                                    << ": not in list of available modules";
                                sendError(str.str());
                                auto ex = make.message<message::ModuleExit>();
                                ex.setSenderId(spawn.spawnId());
                                sendManager(ex);
                            }
                        }
                    }
#endif
                }
                break;
            }

            case message::DEBUG: {
                auto &debug = msg.as<message::Debug>();
                int modId = debug.getModule();
                auto req = debug.getRequest();
                switch (req) {
                case Debug::AttachDebugger: {
                    if (idToHub(modId) != m_hubId) {
                        std::stringstream str;
                        str << "Not trying to debug non-local module id " << modId;
                        sendError(str.str());
                        break;
                    }
#ifdef VISTLE_USE_FMT
                    fmt::dynamic_format_arg_store<fmt::format_context> nargs;
                    {
                        unsigned long rank0pid = 0;
                        if (modId == m_hubId) {
                            rank0pid = m_localManagerRank0Pid;
                        } else {
                            auto it = m_stateTracker.runningMap.find(modId);
                            if (it != m_stateTracker.runningMap.end()) {
                                const auto &mod = it->second;
                                rank0pid = mod.rank0Pid;
                            }
                        }
                        if (rank0pid == 0) {
                            std::stringstream str;
                            str << "Did not find PID of process to attach to for id " << modId;
                            sendError(str.str());
                            break;
                        }
                        nargs.push_back(fmt::arg("rank0pid", rank0pid));
                    }
                    {
#ifdef MODULE_THREAD
                        modId = Process::Manager;
#endif
                        long mpipid = 0;
#ifdef VISTLE_USE_MPI
                        bool found = false;
                        std::unique_lock<std::mutex> guard(m_processMutex);
                        for (auto &p: m_processMap) {
                            if (p.second == modId) {
                                found = true;
                                mpipid = p.first->id();
                                break;
                            }
                        }
                        guard.unlock();
                        if (!found) {
                            std::stringstream str;
                            str << "Did not find launcher PID to debug module id " << modId << " on " << m_name;
                            sendError(str.str());
                            break;
                        }
#endif
                        nargs.push_back(fmt::arg("mpipid", mpipid));
                    }
                    if (auto home = getenv("HOME")) {
                        nargs.push_back(fmt::arg("home", home));
                    } else {
                        sendError("HOME environment variable not set");
                        break;
                    }
                    if (auto user = getenv("USER")) {
                        nargs.push_back(fmt::arg("user", user));
                    } else {
                        sendError("USER environment variable not set");
                        break;
                    }

                    decltype(m_config->array<std::string>("", "", "")) arr;
                    if (!m_hasUi) {
                        std::string suffix = "_batch";
                        arr = m_config->array<std::string>("system", "debugger", platform + suffix);
                        if (arr->size() == 0 && !platform_fallback.empty()) {
                            arr = m_config->array<std::string>("system", "debugger", platform_fallback + suffix);
                        }
                    }
                    if (m_hasUi || arr->size() == 0) {
                        arr.reset();
                        arr = m_config->array<std::string>("system", "debugger", platform);
                        if (arr->size() == 0 && !platform_fallback.empty()) {
                            arr = m_config->array<std::string>("system", "debugger", platform_fallback);
                        }
                    }
                    if (arr->size() == 0) {
                        std::stringstream info;
                        info << "no debugger configured in \"system: [debugger]\"";
                        sendInfo(info.str());
                        break;
                    }
                    std::vector<std::string> cmd_args;
                    for (auto &arg: arr->value()) {
                        cmd_args.push_back(fmt::vformat(arg, nargs));
                    }
                    auto cmd = cmd_args[0];
                    auto args = std::vector<std::string>(cmd_args.begin() + 1, cmd_args.end());
                    auto child = launchProcess(Process::Debugger, cmd, args);
                    if (child) {
                        std::stringstream info;
                        info << "Debugging " << debug.getModule() << " with " << cmd << " as PID " << child->id();
                        sendInfo(info.str());
                    }
#else
                    std::stringstream info;
                    info << "fmt library not available, no support for reading debugger configuration";
                    sendInfo(info.str());
#endif
                    break;
                }
                case Debug::PrintState: {
                    break;
                }
                case Debug::ReplayOutput: {
                    if (modId == m_hubId) {
                        modId = Process::Manager;
                    }
                    const ObservedChild *obs = nullptr;
                    for (auto &ch: m_observedChildren) {
                        if (ch.second.moduleId == modId) {
                            obs = &ch.second;
                            break;
                        }
                    }
                    if (obs) {
                        obs->sendOutputToUi();
                    }
                    break;
                }
                case Debug::SwitchOutputStreaming: {
                    if (modId == m_hubId) {
                        modId = Process::Manager;
                    }
                    ObservedChild *obs = nullptr;
                    for (auto &ch: m_observedChildren) {
                        if (ch.second.moduleId == modId) {
                            obs = &ch.second;
                            break;
                        }
                    }
                    if (obs) {
                        switch (debug.getSwitchAction()) {
                        case Debug::SwitchAction::SwitchOn: {
                            obs->setOutputStreaming(true);
                            break;
                        }
                        case Debug::SwitchAction::SwitchOff: {
                            obs->setOutputStreaming(false);
                            break;
                        }
                        }
                    }
                    break;
                }
                }
                break;
            }

            case message::SETID: {
                assert(!m_isMaster);
                auto &set = static_cast<const SetId &>(msg);
                m_hubId = set.getId();
                m_stateTracker.setId(m_hubId);
                params.setId(m_hubId);
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
                }
                if (m_managerConnected) {
#if 0
               auto state = m_stateTracker.getLockedState();
               for (auto &m: state.messages) {
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

            case message::EXECUTIONDONE: {
                auto &done = static_cast<const ExecutionDone &>(msg);
                handlePriv(done);
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
            case COLORMAP: {
                auto &cmap = msg.as<Colormap>();
                handlePriv(cmap, payload);
                break;
            }
            case REMOVECOLORMAP: {
                auto &rcm = msg.as<RemoveColormap>();
                handlePriv(rcm);
                break;
            }
            default: {
                break;
            }
            }
        }

        return true;
    } catch (vistle::exception &e) {
        CERR << "handleMessage: exception handling " << msg << ": " << e.what() << e.info() << e.where() << std::endl;
    } catch (std::exception &e) {
        CERR << "handleMessage: exception handling " << msg << ": " << e.what() << std::endl;
    } catch (...) {
        CERR << "handleMessage: unknown exception handling " << msg << std::endl;
    }
    return false;
}

bool Hub::spawnMirror(int hubId, const std::string &name, int mirroredId, int blueprintId)
{
    assert(m_isMaster);

    message::Spawn mirror(hubId, name);
    //mirror.setReferrer(spawn.uuid());
    //mirror.setSenderId(m_hubId);
    mirror.setSpawnId(Id::ModuleBase + m_moduleCount);
    ++m_moduleCount;
    mirror.setMirroringId(mirroredId);
    mirror.setDestId(Id::Broadcast);
    m_stateTracker.handle(mirror, nullptr);
    CERR << "sendAll mirror: " << mirror << std::endl;
    sendAll(mirror);
    mirror.setDestId(hubId);
    CERR << "doSpawn: sendManager mirror: " << mirror << std::endl;
    sendManager(mirror, hubId);

    cacheModuleValues(blueprintId, mirror.spawnId());

    auto paramNames = m_stateTracker.getParameters(blueprintId);
    for (const auto &pn: paramNames) {
        auto con = message::Connect(blueprintId, pn, mirror.spawnId(), pn);
        m_sendAfterSpawn[mirror.spawnId()].emplace_back(con);
    }

    return true;
}

bool Hub::handlePriv(const message::LoadWorkflow &load)
{
    std::cerr << "to load: " << load.pathname() << std::endl;
    m_uiManager.lockUi(true);
    auto mods = m_stateTracker.getRunningList();
    for (auto id: mods) {
        message::Kill m(id);
        m.setDestId(id);
        sendModule(m, id);
    }
    assert(!m_lastModuleQuitAction);
    auto act = [this, load]() {
        bool result = processScript(load.pathname(), true, false);
        m_uiManager.lockUi(false);
        return result;
    };
    if (m_stateTracker.getNumRunning() == 0) {
        return act();
    }

    m_numRunningModules = m_stateTracker.getNumRunning();
    m_lastModuleQuitAction = act;
    return true;
}

bool Hub::handlePriv(const message::SaveWorkflow &save)
{
    std::cerr << "to save: " << save.pathname() << std::endl;
    m_uiManager.lockUi(true);
    std::string cmd = "save(\"";
    cmd += save.pathname();
    cmd += "\")";
    bool result = processCommand(cmd);
    m_uiManager.lockUi(false);
    return result;
}


bool Hub::handlePlainSpawn(message::Spawn &notify, bool doSpawn, bool error)
{
    if (error) {
        notify.setSpawnId(Id::Invalid);
    }
    if (m_verbose >= Verbosity::Modules) {
        CERR << "sendManager: " << notify << std::endl;
    }
    notify.setDestId(Id::Broadcast);
    m_stateTracker.handle(notify, nullptr);
    sendAll(notify);
    if (!error && doSpawn) {
        notify.setDestId(notify.hubId());
        if (m_verbose >= Verbosity::Modules) {
            CERR << "doSpawn: sendManager: " << notify << std::endl;
        }
        sendManager(notify, notify.hubId());
    }
    return true;
}
bool Hub::notifySpawnError(message::Spawn &notify)
{
    return handlePlainSpawn(notify, false, true);
}

bool Hub::handlePriv(const message::Spawn &spawn)
{
    std::string moduleName(spawn.getName());
    bool isCover = moduleName == "COVER";

    auto notify = spawn;
    if (spawn.hubId() == m_hubId) {
        if (isCover && m_coverIsManager) {
            notify.setAsPlugin(true);
        }
    }

    if (!m_isMaster) {
        if (m_verbose >= Verbosity::Normal) {
            CERR << "SLAVE: handle spawn: " << spawn << std::endl;
        }
        if (spawn.spawnId() == Id::Invalid) {
            return sendMaster(spawn);
        }

        m_stateTracker.handle(notify, nullptr);
        sendManager(notify);
        sendUi(notify);
        return true;
    }

    notify.setReferrer(spawn.uuid());
    notify.setSenderId(m_hubId);

    auto hubs = m_stateTracker.getHubs();
    if (std::find(hubs.begin(), hubs.end(), spawn.hubId()) == hubs.end()) {
        std::stringstream str;
        str << "hub with id " << spawn.hubId() << " not known";
        sendError(str.str());
        return notifySpawnError(notify);
    }

    bool shouldMirror = isCover && m_stateTracker.getHubData(spawn.hubId()).hasUi;
    bool migrateOrRestart = spawn.getReferenceType() == message::Spawn::ReferenceType::Migrate;
    bool clone = spawn.getReferenceType() == message::Spawn::ReferenceType::Clone;
    bool cloneLinked = spawn.getReferenceType() == message::Spawn::ReferenceType::LinkedClone;

    bool migrate = false;
    bool restart = false;
    if (migrateOrRestart && spawn.getReference() != Id::Invalid) {
        int oldHub = m_stateTracker.getHub(spawn.getReference());
        restart = oldHub == spawn.hubId();
        migrate = !restart;
    }
    bool someFormOfCopy = restart || migrate || clone || cloneLinked;

    bool doSpawn = false;
    int newId = spawn.spawnId();
    int mirroringId = Id::Invalid;
    if (spawn.spawnId() == Id::Invalid) {
        newId = Id::ModuleBase + m_moduleCount;
        ++m_moduleCount;
        notify.setSpawnId(newId);
        if (shouldMirror)
            notify.setMirroringId(newId);
        std::string suffix = "[" + std::to_string(newId) + "]";

        session.setCurrentParameterGroup("Workflow", false);
        session.addIntParameter("layer" + suffix, "layer for module " + std::to_string(newId), Integer(0));
        session.addVectorParameter("position" + suffix, "position for module " + std::to_string(newId),
                                   ParamVector(0., 0.));
        session.setCurrentParameterGroup("", false);

        doSpawn = true;
    } else if (someFormOfCopy) {
        doSpawn = true;
        someFormOfCopy = false;
    }

    if (shouldMirror) {
        mirroringId = newId;
        if (restart) {
            mirroringId = m_stateTracker.getMirrorId(spawn.getReference());
            CERR << "restarting module " << spawn.getReference() << " as " << newId << " with mirroring id "
                 << mirroringId << std::endl;
            CERR << "restart " << notify << std::endl;
        }
        notify.setMirroringId(mirroringId);
    }

    if (someFormOfCopy) {
        if (migrateOrRestart) {
            if (doSpawn) {
                if (!cacheModuleValues(spawn.getReference(), newId)) {
                    sendError("cannot migrate module with id " + std::to_string(spawn.getReference()));
                    return notifySpawnError(notify);
                }
                if (!editDelayedConnects(spawn.getReference(), newId)) {
                    sendError("cannot migrate module with id " + std::to_string(spawn.getReference()));
                    return notifySpawnError(notify);
                }
            }
            m_sendAfterExit[spawn.getReference()].push_back(notify);
            killOldModule(spawn.getReference());
            return true;
        } else if (clone) {
            if (doSpawn) {
                if (!copyModuleParams(spawn.getReference(), newId, true)) {
                    sendError("cannot clone module with id " + std::to_string(spawn.getReference()));
                    return notifySpawnError(notify);
                }
            }
            return handleMessage(notify);
        } else if (cloneLinked) {
            if (doSpawn) {
                if (!linkModuleParams(spawn.getReference(), newId)) {
                    sendError("cannot clone module with id " + std::to_string(spawn.getReference()));
                    return notifySpawnError(notify);
                }
            }
            return handleMessage(notify);
        }
    }

    handlePlainSpawn(notify, doSpawn, false);

    if (doSpawn && shouldMirror && !restart) {
        for (auto &hubid: m_stateTracker.getHubs()) {
            if (hubid == spawn.hubId())
                continue;
            if (hubid == Id::MasterHub && m_isMaster && m_proxyOnly)
                continue;
            const auto &hub = m_stateTracker.getHubData(hubid);
            if (!hub.hasUi)
                continue;

            spawnMirror(hubid, spawn.getName(), newId, newId);
        }
    }
    return true;
}

bool Hub::handlePriv(const message::SetName &setname)
{
    if (m_isMaster || setname.destId() == Id::Broadcast) {
        m_stateTracker.handle(setname, nullptr);
        sendUi(setname);
        sendManager(setname, Id::LocalHub);
    } else if (m_isMaster) {
        auto buf = setname;
        buf.setDestId(Id::Broadcast);
        sendUi(setname);
        sendManager(setname, Id::LocalHub);
        sendSlaves(buf, true);
    }
    return true;
}

template<typename ConnMsg>
bool Hub::handleConnectOrDisconnect(const ConnMsg &mm)
{
    if (m_isMaster) {
#if 0
             if (mm.isNotification()) {
                 CERR << "discarding notification on master: " << mm << std::endl;
                 return true;
             }
#endif
        if (m_stateTracker.handleConnectOrDisconnect(mm)) {
            handlePriv(mm);
            return handleQueue();
        } else {
            queueMessage(mm);
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
}

namespace {
template<typename Msg>
void updateSourceOrDestinationId(Msg &m, int oldId, int newId)
{
    if (m.getModuleA() == oldId) {
        m.setModuleA(newId);
    }
    if (m.getModuleB() == oldId) {
        m.setModuleB(newId);
    }
}
} // namespace

bool Hub::updateQueue(int oldId, int newId)
{
    using namespace message;

    std::unique_lock guard(m_queueMutex);
    for (auto &m: m_queue) {
        if (m.type() == message::CONNECT) {
            auto &mm = m.as<Connect>();
            updateSourceOrDestinationId(mm, oldId, newId);
        } else if (m.type() == message::DISCONNECT) {
            auto &mm = m.as<Disconnect>();
            updateSourceOrDestinationId(mm, oldId, newId);
        }
    }

    return true;
}

void Hub::queueMessage(const message::Buffer &msg)
{
    std::unique_lock guard(m_queueMutex);
    m_queue.push_back(msg);
}

bool Hub::cleanQueue(int id)
{
    using namespace message;

    std::unique_lock guard(m_queueMutex);
    decltype(m_queue) queue;
    std::swap(queue, m_queue);
    guard.unlock();

    for (auto &m: queue) {
        if (m.type() == message::CONNECT) {
            auto &mm = m.as<Connect>();
            if (mm.getModuleA() == id || mm.getModuleB() == id) {
                continue;
            }
        } else if (m.type() == message::DISCONNECT) {
            auto &mm = m.as<Disconnect>();
            if (mm.getModuleA() == id || mm.getModuleB() == id) {
                continue;
            }
        }

        guard.lock();
        m_queue.push_back(m);
        guard.unlock();
    }
    return true;
}

bool Hub::handleQueue()
{
    using namespace message;

    //CERR << "unqueuing " << m_queue.size() << " messages" << std::endl;

    bool again = true;
    while (again) {
        again = false;
        std::unique_lock guard(m_queueMutex);
        decltype(m_queue) queue;
        std::swap(queue, m_queue);
        guard.unlock();
        for (auto &m: queue) {
            if (m.type() == message::CONNECT) {
                auto &mm = m.as<Connect>();
                if (m_stateTracker.handleConnectOrDisconnect(mm)) {
                    again = true;
                    handlePriv(mm);
                    //CERR << "handleQueue: CONNECT now: " << mm << std::endl;
                } else {
                    guard.lock();
                    m_queue.push_back(m);
                    guard.unlock();
                    //CERR << "handleQueue: CONNECT later: " << mm << std::endl;
                }
            } else if (m.type() == message::DISCONNECT) {
                auto &mm = m.as<Disconnect>();
                if (m_stateTracker.handleConnectOrDisconnect(mm)) {
                    again = true;
                    handlePriv(mm);
                    //CERR << "handleQueue: DISCONNECT now: " << mm << std::endl;
                } else {
                    //CERR << "handleQueue: DISCONNECT later: " << m << std::endl;
                    guard.lock();
                    m_queue.push_back(m);
                    guard.unlock();
                }
            } else {
                std::cerr << "message other than Connect/Disconnect in queue: " << m << std::endl;
                guard.lock();
                m_queue.push_back(m);
                guard.unlock();
            }
        }
    }

    return true;
}

bool Hub::startCleaner()
{
#if defined(MODULE_THREAD) && defined(NO_SHMEM)
    return true;
#endif
    if (m_proxyOnly)
        return true;

#ifdef SHMDEBUG
    CERR << "SHMDEBUG: preserving shm for session " << Shm::instanceName(hostname(), m_port) << std::endl;
    return true;
#endif

    if (getenv("SLURM_JOB_ID")) {
        CERR << "not starting under clean_vistle - running SLURM" << std::endl;
        return false;
    }

    // run clean_vistle on cluster
    std::string cmd = m_dir->bin() + "clean_vistle";
    std::vector<std::string> args;
    args.push_back(cmd);
    std::string shmname = Shm::instanceName(hostname(), m_port);
    args.push_back(shmname);
    auto child = launchMpiProcess(Process::Cleaner, args);
    if (!child) {
        CERR << "failed to spawn clean_vistle" << std::endl;
        return false;
    }
    return true;
}

bool Hub::isCachable(int oldModuleId, int newModuleId)
{
    if (!Id::isModule(oldModuleId))
        return false;
    assert(Id::isModule(oldModuleId));
    auto running = m_stateTracker.getRunningList();
    if (std::find(running.begin(), running.end(), oldModuleId) == running.end())
        return false;
    return true;
}

void Hub::cacheParameters(int oldModuleId, int newModuleId, bool clone)
{
    auto paramNames = m_stateTracker.getParameters(oldModuleId);
    for (const auto &pn: paramNames) {
        auto p = m_stateTracker.getParameter(oldModuleId, pn);
        if (!p->isDefault()) {
            auto pm = make.message<message::SetParameter>(newModuleId, p->getName(), p);
            pm.setDelayed();
            pm.setDestId(newModuleId);
            m_sendAfterSpawn[newModuleId].emplace_back(pm);
        }
    }

    restoreModulePosition(oldModuleId, newModuleId, clone);
}

void Hub::restoreModulePosition(int oldModuleId, int newModuleId, bool clone)
{
    std::string suffix = "[" + std::to_string(oldModuleId) + "]";
    std::string nsuffix = "[" + std::to_string(newModuleId) + "]";
    for (auto pnbase: {"position", "layer"}) {
        auto po = std::string(pnbase) + suffix;
        auto pn = std::string(pnbase) + nsuffix;
        auto p = session.findParameter(po);
        if (p && (!p->isDefault() || (clone && pnbase == std::string("position")))) {
            auto pm = message::SetParameter(newModuleId, pn, p);
            if (auto vp = std::dynamic_pointer_cast<VectorParameter>(p)) {
                if (clone) {
                    auto pos = vp->getValue();
                    pos[0] -= m_gridSpacingX;
                    pos[1] += m_gridSpacingY;
                    pm = message::SetParameter(newModuleId, pn, pos);
                }
            }

            pm.setDelayed();
            pm.setDestId(session.id());
            m_sendAfterSpawn[newModuleId].emplace_back(pm);
        }
    }
}

void Hub::cachePortConnections(int oldModuleId, int newModuleId)
{
    auto inputs = m_stateTracker.portTracker()->getConnectedInputPorts(oldModuleId);
    for (const auto &in: inputs) {
        for (const auto &from: in->connections()) {
            auto cm = message::Connect(from->getModuleID(), from->getName(), newModuleId, in->getName());
            m_sendAfterSpawn[newModuleId].emplace_back(cm);
        }
    }

    auto outputs = m_stateTracker.portTracker()->getConnectedOutputPorts(oldModuleId);
    for (const auto &out: outputs) {
        for (const auto &to: out->connections()) {
            auto cm = message::Connect(newModuleId, out->getName(), to->getModuleID(), to->getName());
            m_sendAfterSpawn[newModuleId].emplace_back(cm);
        }
    }

    auto session = m_stateTracker.portTracker()->getConnectedParameters(oldModuleId);
    for (const auto &par: session) {
        for (const auto &to: par->connections()) {
            auto cm = message::Connect(newModuleId, par->getName(), to->getModuleID(), to->getName());
            m_sendAfterSpawn[newModuleId].emplace_back(cm);
        }
    }
}

void Hub::cacheParamConnections(int oldModuleId, int newModuleId)
{
    auto paramNames = m_stateTracker.getParameters(oldModuleId);
    for (const auto &pn: paramNames) {
        auto cm = message::Connect(oldModuleId, pn, newModuleId, pn);
        m_sendAfterSpawn[newModuleId].emplace_back(cm);
    }
}

void Hub::applyAllDelayedParameters(int oldModuleId, int newModuleId)
{
    auto pm = message::SetParameter(newModuleId);
    pm.setDestId(newModuleId);
    m_sendAfterSpawn[newModuleId].emplace_back(pm);
}

bool Hub::editDelayedConnects(int oldModuleId, int newModuleId)
{
    if (m_verbose >= Verbosity::Manager) {
        CERR << "updating connects: " << oldModuleId << " -> " << newModuleId << std::endl;
    }
    for (auto &m: m_sendAfterSpawn) {
        for (auto &msg: m.second) {
            if (msg.type() == message::CONNECT) {
                auto &cm = msg.as<message::Connect>();
                updateSourceOrDestinationId(cm, oldModuleId, newModuleId);
            }
        }
    }
    return updateQueue(oldModuleId, newModuleId);
}

bool Hub::cacheModuleValues(int oldModuleId, int newModuleId)
{
    if (!stateTracker().getModuleDisplayName(oldModuleId).empty()) {
        auto setname = make.message<message::SetName>(newModuleId, stateTracker().getModuleDisplayName(oldModuleId));
        m_sendAfterSpawn[newModuleId].emplace_back(setname);
    }
    if (!copyModuleParams(oldModuleId, newModuleId))
        return false;
    cachePortConnections(oldModuleId, newModuleId);
    return true;
}

bool Hub::copyModuleParams(int oldModuleId, int newModuleId, bool clone)
{
    if (!Id::isModule(oldModuleId))
        return false;
    cacheParameters(oldModuleId, newModuleId, clone);
    applyAllDelayedParameters(oldModuleId, newModuleId);
    return true;
}

bool Hub::linkModuleParams(int oldModuleId, int newModuleId)
{
    if (!copyModuleParams(oldModuleId, newModuleId, true))
        return false;
    cacheParamConnections(oldModuleId, newModuleId);
    return true;
}

void Hub::killOldModule(int migratedId)
{
    assert(Id::isModule(migratedId));
    auto state = m_stateTracker.getModuleState(migratedId);
    if (m_verbose >= Verbosity::Modules) {
        CERR << "restart requested for " << migratedId << ", state=" << state << std::endl;
    }
    if (state & StateObserver::Crashed) {
        auto m = make.message<message::ModuleExit>();
        m.setSenderId(migratedId);
        sendManager(m); // will be returned and forwarded to master hub
    } else {
        message::Kill kill(migratedId);
        kill.setDestId(migratedId);
        sendModule(kill, migratedId);
    }
}


void Hub::sendInfo(const std::string &s, int senderId)
{
    if (m_verbose >= Verbosity::Modules) {
        CERR << s << std::endl;
    }
    auto t = make.message<message::SendText>(message::SendText::Info);
    if (senderId != Id::Invalid) {
        t.setSenderId(senderId);
    }
    message::SendText::Payload pl(s);
    auto payload = addPayload(t, pl);
    sendUi(t, Id::Broadcast, &payload);
}

void Hub::sendError(const std::string &s, int senderId)
{
    if (m_verbose >= Verbosity::Normal) {
        CERR << "Error: " << s << std::endl;
    }
    auto t = make.message<message::SendText>(message::SendText::Error);
    if (senderId != Id::Invalid) {
        t.setSenderId(senderId);
    }
    message::SendText::Payload pl(s);
    auto payload = addPayload(t, pl);
    sendUi(t, Id::Broadcast, &payload);
}


std::vector<int> Hub::getSubmoduleIds(int modId, const AvailableModule &av)
{
    std::vector<int> retval;
    if (av.isCompound()) {
        for (size_t i = 0; i < av.submodules().size(); i++) {
            retval.push_back(modId + i + 1);
        }
    }
    return retval;
}


void Hub::setLoadedFile(const std::string &file)
{
    if (m_verbose >= Verbosity::Modules) {
        CERR << "Loaded file: " << file << std::endl;
    }
    auto t = make.message<message::UpdateStatus>(message::UpdateStatus::LoadedFile, file);
    m_stateTracker.handle(t, nullptr);
    sendUi(t);
}

void Hub::setSessionUrl(const std::string &url)
{
    m_sessionUrl = url;
    if (verbosity() >= Verbosity::Normal) {
        std::cerr << "Share this: " << url << std::endl;
        std::cerr << std::endl;
    }
    auto t = make.message<message::UpdateStatus>(message::UpdateStatus::SessionUrl, url);
    m_stateTracker.handle(t, nullptr);
    sendUi(t);
    if (m_managerConnected) {
        sendManager(t);
    }
    if (m_isMaster) {
        sendSlaves(t);
    }
}

void Hub::setStatus(const std::string &s, message::UpdateStatus::Importance prio)
{
    if (m_verbose >= Verbosity::Normal) {
        CERR << "Status: " << s << std::endl;
    }
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

    CERR << "connecting to principal hub at " << host << ":" << port << std::flush;
    bool connected = false;
    boost::system::error_code ec;
    while (!connected) {
        asio::ip::tcp::resolver resolver(m_ioContext);
        m_masterSocket.reset(new boost::asio::ip::tcp::socket(m_ioContext));
        auto endpoints = resolver.resolve(host, std::to_string(port), ec);
        if (ec) {
            break;
        }
        asio::connect(*m_masterSocket, endpoints, ec);
        if (m_interrupt) {
            break;
        }
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
    if (m_vrb || m_vrbPort) {
        stopVrb();
    }

    auto parseOutput = [this](std::shared_ptr<process::child> child, std::shared_ptr<process::ipstream> stream) {
        std::string line;
        while (child->running() && std::getline(*stream, line) && !line.empty() && m_vrbPort == 0) {
            m_vrbPort = std::atol(line.c_str());
        }
        if (m_vrbPort == 0) {
            CERR << "started VRB process, but could not parse VRB port \"" << line << "\"" << std::endl;
            child->terminate();
            return false;
        }
        if (m_verbose >= Verbosity::Manager && child->running()) {
            CERR << "VRB process running on port " << m_vrbPort << std::endl;
        }
        return child->running();
    };

    if (m_vrbMode == VrbMode::VrbTui) {
        m_vrb = launchProcess(Process::VRB, "vrb", {"--tui", "--printport"}, "vrb", parseOutput);
    } else if (m_vrbMode == VrbMode::VrbGui) {
        m_vrb = launchProcess(Process::VRB, "vrb", {"--printport"}, "vrb", parseOutput);
    }

    if (!m_vrb) {
        return false;
    }

    return true;
}

void Hub::stopVrb()
{
    if (m_vrb && m_vrb->valid()) {
        m_vrb->terminate();
    }
    m_vrb.reset();

    m_vrbPort = 0;

    for (auto &s: m_vrbSockets) {
        removeSocket(s.second);
    }
}

Hub::socket_ptr Hub::connectToVrb(unsigned short port)
{
    assert(m_isMaster);

    socket_ptr sock;
    if (m_quitting || m_interrupt) {
        CERR << "refusing to connect to local VRB at port " << port << ": shutting down" << std::endl;
        return sock;
    }

    if (m_verbose >= Verbosity::Manager) {
        CERR << "connecting to local VRB at port " << port << std::flush;
    }
    bool connected = false;
    boost::system::error_code ec;
    while (!connected) {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address_v4("127.0.0.1"), port);
        sock = std::make_shared<socket>(m_ioContext);
        sock->connect(endpoint, ec);
        if (m_interrupt) {
            sock.reset();
            return sock;
        }
        if (!ec) {
            connected = true;
        } else if (ec == boost::system::errc::connection_refused) {
            if (m_verbose >= Verbosity::Manager) {
                std::cerr << "." << std::flush;
            }
            sleep(1);
        } else {
            if (m_verbose < Verbosity::Manager) {
                CERR << "failed to connect to local VRB at port " << port << ": " << ec.message() << std::endl;
            } else {
                std::cerr << ": connect failed: " << ec.message() << std::endl;
            }
            sock.reset();
            return sock;
        }
    }

    if (m_verbose >= Verbosity::Manager) {
        std::cerr << ": done." << std::endl;
    }

    // we pose as a covise::ClientConnection - cf. covise/src/kernel/net/covise_connect.cpp: ClientConnection::ClientConnection
    const char DF_IEEE = 1;
    char df = 0;
    asio::write(*sock, asio::const_buffer(&DF_IEEE, 1), ec);
    if (ec) {
        CERR << "failed to write data format, resetting socket" << std::endl;
        sock.reset();
        return sock;
    }
    if (m_verbose >= Verbosity::Manager) {
        CERR << "reading VRB data format" << std::flush;
    }
    asio::read(*sock, asio::mutable_buffer(&df, 1), ec);
    if (ec) {
        std::cerr << std::endl;
        CERR << "failed to read data format, resetting socket" << std::endl;
        sock.reset();
        return sock;
    }
    if (m_verbose >= Verbosity::Manager) {
        std::cerr << ": done." << std::endl;
    }
    if (df != DF_IEEE) {
        CERR << "incompatible data format " << static_cast<int>(df) << ", resetting socket" << std::endl;
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

bool Hub::startManager(bool inCover, const std::string &libsimPath)
{
    // start manager on cluster
    std::string cmd = m_dir->bin() + "vistle_manager";
    if (inCover) {
#ifdef VISTLE_USE_MPI
        cmd = "OpenCOVER.mpi";
#else
        cmd = "opencover";
#endif
        setenv("VISTLE_PLUGIN", "VistleManager", 1);

        m_coverIsManager = true;
    }

    std::vector<std::string> args;
    args.push_back(cmd);
    if (!m_coverIsManager) {
        std::string port = std::to_string(this->port());
        std::string dataport = std::to_string(dataPort());
        std::string datamgrbase = std::to_string(dataManagerBase());

        args.push_back("--from-vistle");
        args.push_back(hostname());
        args.push_back(port);
        args.push_back(dataport);
        args.push_back(datamgrbase);
    }

#ifdef MODULE_THREAD
    if (!libsimPath.empty()) {
        CERR << "starting manager in simulation" << std::endl;
        if (vistle::insitu::libsim::attemptLibSimConnection(libsimPath, args)) {
            sendInfo("Successfully connected to simulation");
        } else {
            CERR << "failed to spawn Vistle manager in the simulation" << std::endl;
            return false;
        }

    } else {
#endif // MODULE_THREAD
        auto child = launchMpiProcess(Process::Manager, args);
        if (!child) {
            CERR << "failed to spawn Vistle manager " << std::endl;
            return false;
        }
#ifdef MODULE_THREAD
    }
#endif // MODULE_THREAD
    return true;
}

bool Hub::startUi(const std::string &uipath, bool replace)
{
    std::string port = std::to_string(this->m_masterPort);

    std::vector<std::string> args;
    if (!replace)
        args.push_back("--from-vistle");
    args.push_back(m_masterHost);
    args.push_back(port);
    if (replace) {
        process::system(uipath, process::args(args));
        exit(0);
        return false;
    }

    auto child = launchProcess(Process::GUI, uipath, args);
    if (!child) {
        CERR << "failed to spawn UI " << uipath << std::endl;
        return false;
    }

    return true;
}

bool Hub::startPythonUi()
{
    std::vector<std::string> python_shells{"ipython", "ipython3", "python", "${Python_EXECUTABLE}"};
    std::string port = std::to_string(this->m_masterPort);

    std::string ipython = "ipython";
    std::vector<std::string> args;
    args.push_back("-i");
    args.push_back("-c");
    std::string cmd = "import vistle; ";
    cmd += "vistle._vistle.sessionConnect(None, \"" + m_masterHost + "\", " + port + "); ";
    cmd += "from vistle import *; ";
    args.push_back(cmd);
    args.push_back("--");
    std::shared_ptr<process::child> child;
    for (const auto &shell: python_shells) {
        auto child = launchProcess(Process::GUI, shell, args);
        if (child) {
            return true;
        }
    }

    CERR << "failed to spawn any Python shell, tried:";
    for (const auto &shell: python_shells) {
        std::cerr << " " << shell;
    }
    std::cerr << std::endl;
    return false;
}


bool Hub::processScript()
{
    using boost::algorithm::ends_with;

    if (m_scriptPath.empty())
        return true;

    if (!ends_with(m_scriptPath, ".vsl") && !ends_with(m_scriptPath, ".py")) {
        boost::system::error_code ec;
        if (!filesystem::exists(m_scriptPath, ec)) {
            for (const auto &ext: {".vsl", ".py"}) {
                if (filesystem::exists(m_scriptPath + ext, ec)) {
                    auto retval = processScript(m_scriptPath + ext, m_barrierAfterLoad, m_executeModules);
                    if (retval) {
                        setLoadedFile(m_scriptPath + ext);
                        return true;
                    }
                }
            }
        }
    }

    auto retval = processScript(m_scriptPath, m_barrierAfterLoad, m_executeModules);
    if (retval) {
        setLoadedFile(m_scriptPath);
    }
    return retval;
}

bool Hub::processScript(const std::string &filename, bool barrierAfterLoad, bool executeModules)
{
    assert(m_uiManager.isLocked());
#ifdef HAVE_PYTHON
    if (!m_python) {
        setStatus("Cannot load " + filename + " - no Python interpreter");
        return false;
    }
    setStatus("Loading " + filename + "...");
    int flags = PythonExecutor::LoadFile;
    if (barrierAfterLoad)
        flags |= PythonExecutor::BarrierAfterLoad;
    if (executeModules)
        flags |= PythonExecutor::ExecuteModules;
    PythonExecutor exec(*m_python, flags, filename);
    bool interrupt = false;
    while (!interrupt && !exec.done()) {
        if (!dispatch()) {
            CERR << "interrupting script execution of " << filename << std::endl;
            interrupt = true;
        }
    }
    if (interrupt || exec.state() != PythonExecutor::Success) {
        setStatus("Loading " + filename + " failed");
        return false;
    }
    setStatus("Loading " + filename + " done");
    return true;
#else
    setStatus("Cannot load " + filename + " - no Python support");
    return false;
#endif
}

bool Hub::processCommand(const std::string &command)
{
    assert(m_uiManager.isLocked());
#ifdef HAVE_PYTHON
    if (!m_python) {
        setStatus("Cannot execute: " + command + " - no Python interpreter");
        return false;
    }
    setStatus("Executing " + command + "...");
    PythonExecutor exec(*m_python, command);
    bool interrupt = false;
    while (!interrupt && !exec.done()) {
        if (!dispatch())
            interrupt = true;
    }
    if (interrupt || exec.state() != PythonExecutor::Success) {
        setStatus("Executing " + command + " failed");
        return false;
    }
    setStatus("Executing " + command + " done");
    return true;
#else
    setStatus("Cannot execute: " + command + " - no Python support");
    return false;
#endif
}

const AvailableModule &getStaticModuleInfo(int modId, StateTracker &state)
{
    auto hubId = state.getHub(modId);
    if (hubId != Id::Invalid) {
        auto it = state.availableModules().find({hubId, state.getModuleName(modId)});
        if (it != state.availableModules().end())
            return it->second;
    }
    throw vistle::exception{"getStaticModuleInfo failed for module with id " + std::to_string(modId)};
}

bool Hub::handlePriv(const message::Quit &quit, message::Identify::Identity senderType)
{
    auto sendQuitToManager = [this, &quit]() {
        if (m_managerConnected)
            sendManager(quit);
    };
    if (quit.id() == Id::Broadcast) {
        if (m_verbose >= Verbosity::Normal) {
            CERR << "quit requested by " << senderType << std::endl;
        }
        m_uiManager.requestQuit();
        if (senderType == message::Identify::UNKNOWN /* script */) {
            if (m_isMaster)
                sendSlaves(quit);
            sendQuitToManager();
            initiateQuit();
            return true;
        } else if (senderType == message::Identify::MANAGER) {
            if (m_isMaster)
                sendSlaves(quit);
            initiateQuit();
        } else if (senderType == message::Identify::HUB) {
            sendQuitToManager();
        } else if (senderType == message::Identify::UI) {
            if (m_isMaster)
                sendSlaves(quit);
            else
                sendMaster(quit);
            sendQuitToManager();
            initiateQuit();
        } else {
            sendSlaves(quit);
            sendQuitToManager();
        }
        return false;
    } else if (quit.id() == m_hubId) {
        if (m_isMaster) {
            CERR << "ignoring request to remove master hub" << std::endl;
        } else {
            CERR << "removal of hub requested by " << senderType << std::endl;
            m_uiManager.requestQuit();
            if (senderType == message::Identify::MANAGER) {
                initiateQuit();
            } else {
                sendQuitToManager();
            }
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
            if (rm.senderId() == hub) {
                removeSlave(hub);
                sendUi(rm);
                sendSlaves(rm);
                return true;
            }
            return sendHub(hub, message::Quit(hub));
        } else {
            return false;
        }
    }

    return false;
}

bool Hub::processStartupScripts()
{
    for (const auto &script: scanStartupScripts()) {
        std::cerr << "loading script " << script << std::endl;
        if (!processScript(script, false, false))
            return false;
    }
    return true;
}

bool Hub::handlePriv(const message::Execute &exec)
{
    if (!m_isMaster)
        return true;

    auto toSend = make.message<message::Execute>(exec);

    bool onlySources = false;
    bool isCompound = false;
    std::vector<int> modules{exec.getModule()}; // execute specified module
    if (!Id::isModule(exec.getModule())) {
        // or all modules that are a source in the dataflow graph
        onlySources = true;
        modules.clear();
        for (auto &mod: m_stateTracker.runningMap) {
            int id = mod.first;
            if (!Id::isModule(id))
                continue;
            modules.push_back(id);
        }
    } else {
        const auto &av = getStaticModuleInfo(exec.getModule(), m_stateTracker);
        if (av.isCompound()) {
            isCompound = true;
            modules = getSubmoduleIds(exec.getModule(), av);
        }
    }
    auto downstream = m_stateTracker.getDownstreamModules(exec);
    for (auto id: downstream) {
        auto blockDownstream = make.message<message::Execute>(exec);
        blockDownstream.setWhat(message::Execute::Upstream);
        blockDownstream.setDestId(id);
        blockDownstream.setModule(id);
        sendModule(blockDownstream, id);
    }
    for (auto id: modules) {
        bool canExec = true;
        auto inputs = m_stateTracker.portTracker()->getInputPorts(id);
        for (auto &input: inputs) {
            if (onlySources && !input->connections().empty())
                canExec = false;
            if (input->flags() & Port::COMBINE_BIT)
                canExec = false;
        }
        if (canExec) {
            toSend.setDestId(message::Id::Broadcast);
            toSend.setModule(id);
            m_stateTracker.handle(toSend, nullptr);
            if (isCompound)
                sendAllButUi(toSend);
            else
                sendAll(toSend);
        }
    }

    return true;
}

bool Hub::handlePriv(const message::ExecutionDone &done)
{
    if (m_isMaster) {
        auto notif(done);
        notif.setDestId(Id::Broadcast);
        sendAll(notif);
        handleQueue();
    }

    return true;
}

bool Hub::handlePriv(const message::CancelExecute &cancel)
{
    auto toSend = make.message<message::CancelExecute>(cancel);

    if (Id::isModule(cancel.getModule())) {
        const int hub = m_stateTracker.getHub(cancel.getModule());
        const auto &av = getStaticModuleInfo(cancel.getModule(), m_stateTracker);
        if (av.isCompound()) {
            for (auto sub: getSubmoduleIds(cancel.getModule(), av)) {
                toSend.setDestId(sub);
                sendManager(toSend, hub);
            }
        } else {
            toSend.setDestId(cancel.getModule());
            sendManager(toSend, hub);
        }
    }

    return true;
}

namespace {
bool sendBarrierReached(Hub &hub, const message::uuid_t &barrierUuid)
{
    auto r = hub.make.message<message::BarrierReached>(barrierUuid);
    r.setDestId(Id::NextHop);
    hub.stateTracker().handle(r, nullptr);
    hub.sendUi(r);
    hub.sendSlaves(r);
    hub.sendManager(r);
    return true;
}
} // namespace

bool Hub::handlePriv(const message::Barrier &barrier)
{
    if (m_verbose >= Verbosity::Manager) {
        CERR << "Barrier request: " << barrier.uuid() << " by " << barrier.senderId() << ": " << barrier.info()
             << std::endl;
    }
    if (!m_isMaster) {
        return true;
    }

    assert(!m_barrierActive);
    assert(m_reachedSet.empty());
    m_numBarrierParticipants = m_stateTracker.getNumRunning();
    if (m_numBarrierParticipants == 0) {
        sendBarrierReached(*this, barrier.uuid());
    } else {
        m_barrierActive = true;
        m_barrierUuid = barrier.uuid();
        message::Buffer buf(barrier);
        buf.setDestId(Id::NextHop);
        sendSlaves(buf, true);
        sendManager(buf);
    }
    return true;
}

bool Hub::handlePriv(const message::BarrierReached &reached)
{
    if (m_verbose >= Verbosity::Modules || !m_barrierActive) {
        CERR << "Barrier " << reached.uuid() << " (" << m_stateTracker.barrierInfo(reached.uuid()) << ") reached by "
             << reached.senderId() << " (now " << m_reachedSet.size() << " of " << m_stateTracker.getNumRunning()
             << " modules)" << std::endl;
    }
    assert(m_barrierActive);
    assert(m_barrierUuid == reached.uuid());
    // message must be received from local manager and each slave
    if (m_isMaster) {
        m_reachedSet.insert(reached.senderId());
        if (m_reachedSet.size() == m_numBarrierParticipants) {
            m_barrierActive = false;
            m_reachedSet.clear();
            sendBarrierReached(*this, reached.uuid());
        } else {
            if (m_verbose >= Verbosity::Modules) {
                auto running = m_stateTracker.getRunningList();
                CERR << "waiting for:";
                for (auto r: running) {
                    if (m_reachedSet.find(r) == m_reachedSet.end()) {
                        std::cerr << " " << r;
                    }
                }
                std::cerr << std::endl;
            }
        }
    } else {
        if (reached.senderId() == Id::MasterHub) {
            m_barrierActive = false;
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

template<typename ConnMsg>
void Hub::handleMirrorConnect(const ConnMsg &conn, std::function<bool(const message::Message &)> sendFunc)
{
    auto &state = m_stateTracker;

    auto modA = conn.getModuleA();
    auto modB = conn.getModuleB();
    auto portA = state.portTracker()->getPort(modA, conn.getPortAName());
    auto portB = state.portTracker()->getPort(modB, conn.getPortBName());
    if (!portA || !portB)
        return;
    if (portA->getType() == Port::PARAMETER || portB->getType() == Port::PARAMETER) {
        if (const auto param = state.getParameter(modA, portA->getName())) {
            message::SetParameter setParam(modA, portA->getName(), param);
            updateLinkedParameters(setParam);
        }
        return;
    }
    auto mirA = state.getMirrors(modA);
    auto mirB = state.getMirrors(modB);
    auto c = conn;
    c.setNotify(true);
    for (auto a: mirA) {
        if (a == modA)
            continue;
        c.setModuleA(a);
        sendFunc(c);
    }
    c.setModuleA(modA);

    for (auto b: mirB) {
        if (b == modB)
            continue;
        c.setModuleB(b);
        sendFunc(c);
    }
}

template<typename ConnMsg>
bool Hub::handlePrivConnMsg(const ConnMsg &conn, message::MessageFactory &make)
{
    auto modA = conn.getModuleA();
    auto modB = conn.getModuleB();

    const auto &avA = getStaticModuleInfo(modA, stateTracker());
    const auto &avB = getStaticModuleInfo(modB, stateTracker());

    std::vector<Port> portsA, portsB;
    if (avA.isCompound()) {
        for (const auto &subConn: avA.connections()) {
            if (subConn.toId < 0 && conn.getPortAName() == subConn.toPort) {
                portsA.push_back({subConn.fromId + modA + 1, subConn.fromPort, Port::Type::OUTPUT});
            }
        }
    } else {
        portsA.push_back({modA, conn.getPortAName(), Port::Type::OUTPUT});
    }

    if (avB.isCompound()) {
        for (const auto &subConn: avB.connections()) {
            if (subConn.fromId < 0 && conn.getPortBName() == subConn.fromPort) {
                portsB.push_back({subConn.toId + modB + 1, subConn.toPort, Port::Type::INPUT});
            }
        }
    } else {
        portsB.push_back({modB, conn.getPortBName(), Port::Type::INPUT});
    }
    for (const auto &portA: portsA) {
        for (const auto &portB: portsB) {
            modA = portA.getModuleID();
            modB = portB.getModuleID();
            auto connMsg = make.message<ConnMsg>(modA, portA.getName(), modB, portB.getName());
            connMsg.setNotify(true);
            handleMirrorConnect(connMsg, [this](const message::Message &msg) { return sendAll(msg); });
            sendAll(connMsg);
            // handleMirrorConnect(connMsg, [this](const message::Connect &msg) { return sendAllButUi(msg); });
            // sendAllButUi(connMsg);
        }
    }
    auto c = make.message<ConnMsg>(conn);
    c.setNotify(true);
    sendAllUi(c);
    handleMirrorConnect(conn, [this](const message::Message &msg) { return sendAllUi(msg); });

    return true;
}

bool Hub::handlePriv(const message::Connect &conn)
{
    return handlePrivConnMsg(conn, make);
}

bool Hub::handlePriv(const message::Disconnect &disc)
{
    return handlePrivConnMsg(disc, make);
}

bool Hub::handlePriv(const message::FileQuery &query, const buffer *payload)
{
    int hub = m_stateTracker.getHub(query.moduleId());
    if (hub != m_hubId)
        return sendHub(hub, query, payload);

    buffer pl;
    if (!payload)
        payload = &pl;

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
    bool crashed = exit.isCrashed();
    int id = exit.senderId();
    auto it = m_vrbSockets.find(id);
    if (it != m_vrbSockets.end()) {
        removeSocket(it->second);
        m_vrbSockets.erase(it);
    }

    if (m_barrierActive) {
        if (m_verbose >= Verbosity::Manager) {
            CERR << "module " << id << " exited while barrier active" << std::endl;
        }
        auto r = make.message<message::BarrierReached>(m_barrierUuid);
        r.setSenderId(id);
        r.setDestId(Id::MasterHub);
        handleMessage(r);
    }

    bool restarting = false;
    if (!crashed) {
        std::vector<message::Disconnect> disconnects;
        auto inputs = m_stateTracker.portTracker()->getConnectedInputPorts(id);
        for (const auto &in: inputs) {
            for (const auto &from: in->connections()) {
                disconnects.emplace_back(from->getModuleID(), from->getName(), id, in->getName());
            }
        }

        auto outputs = m_stateTracker.portTracker()->getConnectedOutputPorts(id);
        for (const auto &out: outputs) {
            for (const auto &to: out->connections()) {
                disconnects.emplace_back(id, out->getName(), to->getModuleID(), to->getName());
            }
        }
        for (auto &dm: disconnects)
            handleMessage(dm);

        auto removePorts = m_stateTracker.portTracker()->removeModule(exit.senderId());
        for (auto &rm: removePorts)
            handleMessage(rm);

        std::string suffix = "[" + std::to_string(id) + "]";
        for (auto p: {"position", "layer"}) {
            session.removeParameter(p + suffix);
        }

        auto it2 = m_sendAfterExit.find(exit.senderId());
        if (it2 != m_sendAfterExit.end()) {
            for (auto &m: it2->second) {
                handleMessage(m);
            }
            m_sendAfterExit.erase(it2);
            restarting = true;
        }
    }

    if (m_isMaster && !exit.leaveMirrorGroup() && !restarting) {
        const auto &hub = m_stateTracker.getHubData(idToHub(id));
        if (!hub.isQuitting) {
            auto mirrors = m_stateTracker.getMirrors(id);
            for (auto m: mirrors) {
                if (m == id)
                    continue;
                auto kill = message::Kill(m);
                kill.setDestId(m);
                handleMessage(kill);
            }
        }
    }

    cleanQueue(id);

    checkLastModuleQuit();

    return true;
}

bool Hub::checkLastModuleQuit()
{
    if (m_stateTracker.getNumRunning() > 0 || m_numRunningModules == 0) {
        m_numRunningModules = m_stateTracker.getNumRunning();
        return false;
    }

    m_numRunningModules = 0;
    if (m_lastModuleQuitAction) {
        CERR << "executing lastModuleQuitAction... " << std::flush;
        if (m_lastModuleQuitAction()) {
            std::cerr << "ok";
        } else {
            std::cerr << "ERROR";
        }
        std::cerr << std::endl;
        m_lastModuleQuitAction = nullptr;
    }
    m_numRunningModules = m_stateTracker.getNumRunning();
    return true;
}

bool Hub::handlePriv(const message::Kill &kill)
{
    if (m_isMaster) {
        auto id = kill.destId();
        auto state = m_stateTracker.getModuleState(id);
        if (state & StateObserver::Crashed) {
            auto m = make.message<message::ModuleExit>();
            m.setSenderId(id);
            sendManager(m); // will be returned and forwarded to master hub
        }
    }
    return true;
}

bool Hub::handlePriv(const message::Cover &cover, const buffer *payload)
{
    if (!m_isMaster) {
        //CERR << "forwarding: " << cover << std::endl;
        return sendMaster(cover, payload);
    }

    if (m_vrbMode == VrbMode::VrbNo) {
        return false;
    }

    if (m_vrbPort == 0) {
        if (std::chrono::steady_clock::now() - m_lastVrbStart < std::chrono::seconds(m_vrbStartWait)) {
            return false;
        }

        CERR << "restarting VRB on behalf of " << cover.senderId() << "..." << std::endl;
        m_lastVrbStart = std::chrono::steady_clock::now();
        if (!startVrb()) {
            CERR << "failed to restart VRB on behalf of " << cover.senderId() << std::endl;
            m_vrbStartWait *= 2;
            if (m_vrbStartWait > 60) {
                m_vrbStartWait = 60;
            }
            return false;
        }
        m_vrbStartWait = 1;
        assert(m_vrbPort != 0);
    }

    //CERR << "handling: " << cover << std::endl;

    socket_ptr sock;
    auto it = m_vrbSockets.find(cover.senderId());
    if (it == m_vrbSockets.end()) {
        if (m_verbose >= Verbosity::Manager) {
            CERR << "connecting to VRB on port " << m_vrbPort << " on behalf of " << cover.senderId() << std::endl;
        }
        sock = connectToVrb(m_vrbPort);
        if (sock) {
            it = m_vrbSockets.emplace(cover.senderId(), sock).first;
        } else {
            CERR << "failed to connect to VRB on port " << m_vrbPort << " on behalf of " << cover.senderId()
                 << std::endl;
        }
    } else {
        sock = it->second;
        if (!sock) {
            if (m_verbose >= Verbosity::Manager) {
                CERR << "connecting to VRB on port " << m_vrbPort << " on behalf of " << cover.senderId() << std::endl;
            }
            sock = connectToVrb(m_vrbPort);
            if (sock) {
                it->second = sock;
                int destMod = it->first;

                if (m_verbose >= Verbosity::Manager) {
                    CERR << "triggering VRB reconnection for " << destMod << std::endl;
                }
                enum { COVISE_MESSAGE_SOCKET_CLOSED = 84 };
                message::Cover cover(COVISE_MESSAGE_SOCKET_CLOSED);
                cover.setDestId(destMod);
                sendModule(cover, destMod);
            } else {
                CERR << "failed to connect to VRB on port " << m_vrbPort << " on behalf of " << cover.senderId()
                     << std::endl;
            }
        }
    }
    if (sock) {
        assert(it != m_vrbSockets.end());
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

bool Hub::handlePriv(const message::Colormap &cm, const buffer *payload)
{
    return true;
}

bool Hub::handlePriv(const message::RemoveColormap &rmcm)
{
    return true;
}

bool Hub::hasChildProcesses(bool ignoreGui)
{
    std::lock_guard<std::mutex> guard(m_processMutex);
    if (ignoreGui) {
        for (const auto &pk: m_processMap) {
            if (pk.second != Process::GUI && pk.second != Process::VRB)
                return true;
        }

        return false;
    }

    return !m_processMap.empty();
}

bool Hub::checkChildProcesses(bool emergency, bool onMainThread)
{
    bool hasToQuit = false;
    ProcessMap exited;
    std::unique_lock<std::mutex> guard(m_processMutex);
    bool oneDead = false;
    for (auto it = m_processMap.begin(), next = it; it != m_processMap.end(); it = next) {
        next = it;
        ++next;

        if (it->first->running())
            continue;

        oneDead = true;
        exited.emplace(*it);
        next = m_processMap.erase(it);
    }
    guard.unlock();

    while (!exited.empty()) {
        auto it = exited.begin();
        try {
            it->first->wait();
        } catch (const std::exception &e) {
            // probably already waited for together with other processes
            //CERR << "waiting for process with id " << it->second << " terminated with exception: " << e.what() << std::endl;
        }

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

        bool exitOk = false;
        if (id == Process::VRB) {
            if (onMainThread) {
                stopVrb();
            } else {
                if (m_vrbPort == 0) {
                    exitOk = true;
                } else {
                    if (m_verbose >= Verbosity::Manager) {
                        CERR << "VRB on port " << m_vrbPort << " has exited" << std::endl;
                    }
                    stopVrb();
                }
            }
            assert(m_vrbPort == 0);
        } else if (!emergency) {
            if (id == Process::Manager) {
                if (!m_quitting) {
                    // manager died unexpectedly
                    CERR << "manager died - cannot continue" << std::endl;
                    hasToQuit = true;
                }
            } else if (message::Id::isModule(id) && m_stateTracker.getModuleState(id) != StateObserver::Unknown &&
                       m_stateTracker.getModuleState(id) != StateObserver::Quit) {
                // synthesize ModuleExit message for crashed modules
                auto m = make.message<message::ModuleExit>(true /* crashed*/);
                m.setSenderId(id);
                m.setLeaveMirrorGroup(true);
                sendManager(m); // will be returned and forwarded to master hub
            }
        }

        std::string pname;
        const ObservedChild *obs = nullptr;
        auto obsit = m_observedChildren.find(it->first->id());
        if (obsit != m_observedChildren.end()) {
            obs = &obsit->second;
            pname = obs->name + "_" + std::to_string(id) + " (PID " + std::to_string(it->first->id()) + ")";
        } else {
            pname = "with id " + idstring + " (PID " + std::to_string(it->first->id()) + ")";
        }

        if (!exitOk && it->first->exit_code() != 0) {
            bool ok = false;
            std::stringstream str;
#ifdef _WIN32
            str << "process " << pname << " exited with code " << it->first->exit_code();
#else
            auto native = it->first->native_exit_code();
            if (WIFSIGNALED(native)) {
                int sig = WTERMSIG(native);
                str << "process " << pname << " killed by signal " << sig;
            } else if (WIFEXITED(native)) {
                int ec = WEXITSTATUS(native);
                str << "process " << pname << " exited with code " << ec;
                if (ec == 0) {
                    ok = true;
                }
            } else if (WIFSTOPPED(native)) {
                // should not happen
                int sig = WSTOPSIG(native);
                str << "process " << pname << " stopped by signal " << sig;
                ok = true;
            } else {
                str << "process " << pname << " exited abnormally with native code " << native << "/"
                    << it->first->exit_code();
            }
#endif
            if (!ok) {
                sendError(str.str(), id);
                if (obs) {
                    str << std::endl;
                    obs->sendOutputToUi(!message::Id::isModule(id));
                }
            }
        } else {
            if (m_verbose >= Verbosity::Manager) {
                CERR << "process " << pname << " exited" << std::endl;
            }
        }
        if (obsit != m_observedChildren.end()) {
            m_observedChildren.erase(obsit);
        }

        exited.erase(it);
    }

    if (hasToQuit) {
        emergencyQuit();
    }

    return oneDead;
}

void Hub::emergencyQuit()
{
    if (m_emergency)
        return;

    CERR << "forced to quit" << std::endl;
    m_emergency = true;

    for (auto lock = std::unique_lock(m_outstandingDataConnectionMutex); !m_outstandingDataConnections.empty();) {
        lock.unlock();
        checkOutstandingDataConnections();
        lock.lock();
    }

    m_stateTracker.cancel();

    if (m_dataProxy)
        m_dataProxy->cleanUp();

    m_workGuard.reset();
    m_ioContext.stop();
    while (!m_ioContext.stopped()) {
        usleep(100000);
    }

    m_tunnelManager.cleanUp();
    m_dataProxy.reset();

    if (!m_quitting) {
        m_uiManager.requestQuit();
        {
            std::lock_guard<std::mutex> guard(m_processMutex);
            m_processMap.clear();
        }
        if (startCleaner()) {
            initiateQuit();
            while (hasChildProcesses()) {
                if (!checkChildProcesses(true)) {
                    usleep(100000);
                }
            }
        }
        exit(1);
    }
}

const AvailableModule *Hub::findModule(const AvailableModule::Key &key)
{
    const auto &avail = m_stateTracker.availableModules();
    auto it = avail.find(key);
    if (it == avail.end()) {
        return nullptr;
    }

    auto *mod = &it->second;
    return mod;
}

void Hub::spawnModule(const std::string &path, const std::string &name, int spawnId)
{
    std::vector<std::string> argv;
    argv.push_back(path);
    argv.push_back(hostname());
    argv.push_back(Shm::instanceName(hostname(), m_port));
    argv.push_back(name);
    argv.push_back(std::to_string(spawnId));
    if (m_verbose >= Verbosity::Modules) {
        CERR << "starting module " << name << std::endl;
    }
    auto child = launchMpiProcess(message::Id::isModule(spawnId) ? spawnId : Process::Module, argv);
    if (!child) {
        std::stringstream str;
        str << "program " << argv[0] << " failed to start";
        sendError(str.str());
        auto ex = make.message<message::ModuleExit>();
        ex.setSenderId(spawnId);
        ex.setLeaveMirrorGroup(true);
        sendManager(ex);
    }
}

void Hub::updateLinkedParameters(const message::SetParameter &setParam)
{
    // only master hub should deal with parameter connections
    if (!m_isMaster)
        return;

    // the msg should have a referrer if it is in reaction to a connected parameter change
    if (setParam.referrer()
            .is_nil()) { //prevents msgs running in circles if e.g.: parameter bounds prevent them from being equal

        //depends on whether the ui or the module request the parameter change
        //auto moduleID = message::Id::isModule(setParam.destId()) ? setParam.destId() : setParam.senderId();
        auto moduleID = setParam.getModule();
        std::string name = setParam.getName();
        const auto port = m_stateTracker.portTracker()->findPort(moduleID, name);
        const auto param = m_stateTracker.getParameter(moduleID, name);
        std::shared_ptr<Parameter> appliedParam;
        if (param) {
            appliedParam.reset(param->clone());
            setParam.apply(appliedParam);
        }

        if (port && appliedParam) {
            ParameterSet conn = m_stateTracker.getConnectedParameters(*appliedParam);

            for (ParameterSet::iterator it = conn.begin(); it != conn.end(); ++it) {
                const auto p = *it;
                if (p->module() == moduleID && p->getName() == name) {
                    // don't update parameter which was set originally again
                    continue;
                }
                auto set = make.message<message::SetParameter>(p->module(), p->getName(), appliedParam);
                set.setDestId(p->module());
                set.setUuid(setParam.uuid());
                sendAll(set);
            }
        }
    }
}


void Hub::startIoThread()
{
    auto num = m_ioThreads.size();
    if (num > std::thread::hardware_concurrency())
        return;
    m_ioThreads.emplace_back([this, num]() {
        setThreadName("vistle:io:" + std::to_string(num));
        m_ioContext.run();
    });
}

void Hub::stopIoThreads()
{
    m_workGuard.reset();
    m_ioContext.stop();

    while (!m_ioThreads.empty()) {
        auto &t = m_ioThreads.back();
        if (t.joinable())
            t.join();
        m_ioThreads.pop_back();
    }
}

} // namespace vistle
