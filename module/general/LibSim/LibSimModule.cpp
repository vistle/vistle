#include "LibSimModule.h"

#include <vistle/util/hostname.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/sleep.h>

#include <vistle/insitu/libsim/connectLibsim/connect.h>

#include <sstream>
#include <vistle/core/rectilineargrid.h>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/mpi.hpp>

#include <vistle/core/message.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/insitu/core/slowMpi.h>
#include <vistle/insitu/util/print.h>

#ifdef MODULE_THREAD
#include <vistle/insitu/libsim/engineInterface/EngineInterface.h>
#endif
using namespace std;
namespace libsim = vistle::insitu::libsim;
using vistle::insitu::message::InSituMessageType;

#define CERR cerr << "LibSimModule[" << rank() << "/" << size() << "] "
#define DEBUG_CERR vistle::DoNotPrintInstance


LibSimModule::LibSimModule(const string &name, int moduleID, mpi::communicator comm)
: InSituReader(name, moduleID, comm)
, m_socketComm(comm, boost::mpi::comm_create_kind::comm_duplicate)
#ifndef MODULE_THREAD
#if BOOST_VERSION >= 106600
, m_workGuard(boost::asio::make_work_guard(m_ioService))
#else
, m_workGuard(new boost::asio::io_service::work(m_ioService))
#endif
, m_ioThread([this]() {
    m_ioService.run();
    DEBUG_CERR << "io thread terminated" << endl;
})
{
    m_acceptorv4.reset(new boost::asio::ip::tcp::acceptor(m_ioService));
    m_acceptorv6.reset(new boost::asio::ip::tcp::acceptor(m_ioService));

    m_filePath = addStringParameter("path", "path to a .sim2 file or directory containing these files", "",
                                    vistle::Parameter::ExistingFilename);
    setParameterFilters(m_filePath, "simulation Files (*.sim2)");
    m_simName = addStringParameter("simulation name",
                                   "the name of the simulation as used in the filename of the sim2 file ", "");
#else
{
#endif // !MODULE_THREAD

    m_intOptions[addIntParameter("VTKVariables", "sort the variable data on the grid from VTK ordering to Vistles",
                                 false, vistle::Parameter::Boolean)] = libsim::IntOptions::VtkFormat;
    m_intOptions[addIntParameter("constant grids", "are the grids the same for every timestep?", false,
                                 vistle::Parameter::Boolean)] = libsim::IntOptions::ConstGrids;
    m_intOptions[addIntParameter("frequency", "frequency in which data is retrieved from the simulation", 1)] =
        libsim::IntOptions::NthTimestep;
    m_intOptions[addIntParameter("combine grids", "combine all structure grids on a rank to a single unstructured grid",
                                 false, vistle::Parameter::Boolean)] = libsim::IntOptions::CombineGrids;
    m_intOptions[addIntParameter("keep timesteps", "keep data of processed timestep of this execution", true,
                                 vistle::Parameter::Boolean)] = libsim::IntOptions::KeepTimesteps;

#ifndef MODULE_THREAD
    if (rank() == 0) {
        startControllServer(); // start socket and wait for connection
    }
    startSocketThread();
#else
    bool simRunning = true;
    if (rank() == 0 && !insitu::EngineInterface::getControllSocket()) {
        CERR << "can not start without running simulation" << endl;
        simRunning = false;
    }
    boost::mpi::broadcast(comm, simRunning, 0);
    if (!simRunning) {
        return;
    }

    reconnect();
    m_connectedToEngine = true;
    m_messageHandler.initialize(insitu::EngineInterface::getControllSocket(),
                                boost::mpi::communicator(comm, boost::mpi::comm_create_kind::comm_duplicate));
    m_messageHandler.send(vistle::insitu::message::ModuleID{id()});
    m_socketThread = std::thread([this]() {
        while (!getBool(m_terminateSocketThread)) {
            recvAndhandleMessage();
        }
    });
#endif // !MODULE_THREAD
}

LibSimModule::~LibSimModule()
{
    setBool(m_terminateSocketThread, true);
    comm().barrier(); // make sure m_terminate is set on all ranks before releasing slaves from waiting for connection
#ifndef MODULE_THREAD
    if (rank() == 0) {
        boost::system::error_code ec;
        if (getBool(m_connectedToEngine)) {
            m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive); // release me from waiting for messages
            m_messageHandler.send(vistle::insitu::message::ConnectionClosed{false});
            m_socket->close(ec);
        }

        else {
            boost::system::error_code ec;

            m_ListeningSocket->cancel(ec); // release asio from waiting for connection
            {
                Guard g(m_asioMutex);
                m_waitingForAccept = true;
            }
            m_connectedCondition.notify_one(); // release me from waiting for connection
        }
    }

    m_workGuard.reset();
    m_ioService.stop();
    m_ioThread.join();
#endif
    if (m_socketThread.joinable()) {
        m_socketThread.join();
    }
}

bool LibSimModule::endExecute()
{
    m_messageHandler.send(vistle::insitu::message::Ready{false});
    return true;
}

bool LibSimModule::beginExecute()
{
    if (!getBool(m_connectedToEngine)) {
        return true;
    }
    vistle::insitu::message::SetPorts::value_type connectedPorts;
    std::vector<string> p;
    for (const auto &port: m_outputPorts) {
        if (port.second->isConnected()) {
            p.push_back(port.first);
        }
    }
    connectedPorts.push_back(p);
    m_messageHandler.send(vistle::insitu::message::SetPorts{connectedPorts});
    m_messageHandler.send(vistle::insitu::message::Ready{true});

    return true;
}

bool LibSimModule::changeParameter(const vistle::Parameter *param)
{
    Module::changeParameter(param);
    if (!param) {
        return true;
    }
#ifndef MODULE_THREAD
    if (param == m_filePath || param == m_simName) {
        connectToSim();
    } else
#endif

        if (std::find(m_commandParameter.begin(), m_commandParameter.end(), param) != m_commandParameter.end() ||
            std::find(m_customCommandParameter.begin(), m_customCommandParameter.end(), param) !=
                m_customCommandParameter.end()) {
        std::string value;
        if (const auto stringParam = dynamic_cast<const vistle::StringParameter *>(param))
            value = stringParam->getValue();
        m_messageHandler.send(vistle::insitu::message::ExecuteCommand({param->getName(), value}));
    } else {
        auto option = dynamic_cast<const vistle::IntParameter *>(param);
        auto it = m_intOptions.find(option);
        if (it != m_intOptions.end()) {
            m_messageHandler.send(vistle::insitu::message::LibSimIntOption{{it->second, option->getValue()}});
        }
    }

    return InSituReader::changeParameter(param);
}
#ifndef MODULE_THREAD
void LibSimModule::startControllServer()
{
    unsigned short port = m_port;

    boost::system::error_code ec;

    while (!vistle::start_listen(port, *m_acceptorv4, *m_acceptorv6, ec)) {
        if (ec == boost::system::errc::address_in_use) {
            ++port;
        } else if (ec) {
            CERR << "failed to listen on port " << port << ": " << ec.message() << endl;
            return;
        }
    }

    m_port = port;

    CERR << "listening for connections on port " << m_port << endl;
    return;
}

bool LibSimModule::startAccept(shared_ptr<acceptor> a)
{
    if (!a->is_open())
        return false;

    shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(m_ioService));
    m_ListeningSocket = sock;
    boost::system::error_code ec;
    a->async_accept(*sock, [this, a, sock](boost::system::error_code ec) {
        if (ec) {
            m_socket = nullptr;
            CERR << "failed connection attempt" << endl;
            return;
        }
        CERR << "connected with engine" << endl;
        m_socket = sock;
        {
            Guard g(m_asioMutex);
            m_waitingForAccept = true;
        }
        m_connectedCondition.notify_one();
    });
    return true;
}

void LibSimModule::startSocketThread()
{
    m_socketThread = std::thread([this]() {
        resetSocketThread();
        while (!getBool(m_terminateSocketThread)) {
            recvAndhandleMessage();
        }
    });
}

void LibSimModule::resetSocketThread()
{
    if (rank() == 0) {
        startAccept(m_acceptorv4);
        startAccept(m_acceptorv6);
        std::unique_lock<std::mutex> lk(m_asioMutex);
        m_connectedCondition.wait(lk, [this]() { return m_waitingForAccept; });
        m_waitingForAccept = false;
    }
    vistle::insitu::waitForRank(m_socketComm, 0);
    if (getBool(m_terminateSocketThread)) {
        return;
    }
    setBool(m_connectedToEngine, true);
    m_messageHandler.initialize(m_socket, m_socketComm);
    for (const auto &option: m_intOptions) {
        m_messageHandler.send(vistle::insitu::message::LibSimIntOption{{option.second, option.first->getValue()}});
    }
}

void LibSimModule::connectToSim()
{
    if (m_simInitSent || m_connectedToEngine) {
        DEBUG_CERR << "already connected" << endl;
        return;
    }
    reconnect();
    if (rank() == 0) {
        using namespace boost::filesystem;
        path p{m_filePath->getValue()};
        if (is_directory(p)) {
            auto simName = m_simName->getValue();
            path lastEditedFile;
            std::time_t lastEdit{};
            for (auto &entry: boost::make_iterator_range(boost::filesystem::directory_iterator(p), {})) {
                if (simName.size() == 0 ||
                    entry.path().filename().generic_string().find(simName + ".sim2") != std::string::npos) {
                    auto editTime = last_write_time(entry.path());
                    if (editTime > lastEdit) {
                        lastEdit = editTime;
                        lastEditedFile = entry.path();
                    }
                }
            }
            p = lastEditedFile;
        }
        CERR << "opening file: " << p.string() << endl;
        vector<string> args{
            to_string(size()), vistle::Shm::the().instanceName(), name(), to_string(id()), vistle::hostname(),
            to_string(m_port), to_string(instanceNum())};
        if (vistle::insitu::libsim::attemptLibSImConnection(p.string(), args)) {
            m_simInitSent = true;
        } else {
            CERR << "attemptLibSimConnection failed " << endl;
        }
    }
    boost::mpi::broadcast(comm(), m_simInitSent, 0);
}

void LibSimModule::disconnectSim()
{
    if (rank() == 0) {
        CERR << "LibSimController is disconnecting from simulation" << endl;
    }
    {
        Guard g(m_socketMutex);
        m_simInitSent = false;
        m_connectedToEngine = false;
        if (m_terminateSocketThread) {
            return;
        }
    }

    resetSocketThread();
}
#endif // !MODULE_THREAD

void LibSimModule::recvAndhandleMessage()
{
    auto msg = m_messageHandler.recv();

    DEBUG_CERR << "handleMessage " << (int)msg.type() << endl;
    switch (msg.type()) {
    case InSituMessageType::SetPorts: // list of ports, last entry is the type description (e.g mesh or variable)
    {
        auto em = msg.unpackOrCast<vistle::insitu::message::SetPorts>();
        for (auto i = m_outputPorts.begin(); i != m_outputPorts.end(); ++i) { // destoy unnecessary ports
            if (std::find_if(em.value.begin(), em.value.end(), [i](const std::vector<string> &ports) {
                    return std::find(ports.begin(), ports.end(), i->first) != ports.end();
                }) == em.value.end()) {
                destroyPort(i->second);
                i = m_outputPorts.erase(i);
            }
        }
        for (auto portList: em.value) {
            for (size_t i = 0; i < portList.size() - 1; i++) {
                auto lb = m_outputPorts.lower_bound(portList[i]);
                if (!(lb != m_outputPorts.end() && !(m_outputPorts.key_comp()(portList[i], lb->first)))) {
                    m_outputPorts.insert(
                        lb, std::make_pair(portList[i], createOutputPort(portList[i], portList[portList.size() - 1])));
                }
            }
        }

    } break;
    case InSituMessageType::SetCommands:
    case InSituMessageType::SetCustomCommands: {
        auto commands = msg.type() == InSituMessageType::SetCommands
                            ? msg.unpackOrCast<vistle::insitu::message::SetCommands>().value
                            : msg.unpackOrCast<vistle::insitu::message::SetCustomCommands>().value;
        auto &params = msg.type() == InSituMessageType::SetCommands ? m_commandParameter : m_customCommandParameter;

        for (auto i = params.begin(); i != params.end(); ++i) {
            if (std::find(commands.begin(), commands.end(), (*i)->getName()) == commands.end()) {
                removeParameter(*i);
                i = params.erase(i);
            }
        }
        for (auto cmd: commands) {
            auto lb =
                std::find_if(params.begin(), params.end(), [cmd](const auto &val) { return val->getName() == cmd; });
            if (lb == params.end()) {
                if (msg.type() == InSituMessageType::SetCommands)
                    params.insert(addIntParameter(cmd, "trigger command on change", false,
                                                  vistle::Parameter::Presentation::Boolean));
                else
                    params.insert(addStringParameter(cmd, "trigger command on change", ""));
            }
        }
    } break;
    case InSituMessageType::GoOn: {
        m_messageHandler.send(vistle::insitu::message::GoOn{});
    } break;
    case InSituMessageType::ConnectionClosed: {
        auto state = msg.unpackOrCast<vistle::insitu::message::ConnectionClosed>();
        if (state.value) {
            CERR << "the simulation disconnected properly" << endl;
        } else {
            CERR << " tcp connection closed...disconnecting." << endl;
        }
#ifdef MODULE_THREAD
        sendMessage(vistle::message::Quit());
#else
        disconnectSim();
#endif
    } break;
    default:
        break;
    }
    vistle::adaptive_wait(msg.type() != InSituMessageType::Invalid);
}

void LibSimModule::setBool(bool &target, bool newval)
{
    Guard g(m_socketMutex);
    target = newval;
}

bool LibSimModule::getBool(const bool &val)
{
    Guard g(m_socketMutex);
    return val;
}

MODULE_MAIN_THREAD(LibSimModule, boost::mpi::threading::multiple)
