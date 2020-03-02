#include "LibSimModule.h"

#include <util/sleep.h>
#include <util/listenv4v6.h>
#include <util/hostname.h>

#include <insitu/libsim/EstablishConnection.h>

#include <core/rectilineargrid.h>
#include <sstream>

#include <boost/bind.hpp>


#include <core/message.h>
#include <core/tcpmessage.h>

#include <boost/filesystem.hpp>
#include <util/print.h>
using namespace std;
using insitu::message::InSituTcpMessage;
using insitu::message::SyncShmMessage;
using insitu::message::InSituMessageType;

#define CERR cerr << "LibSimModule["<< rank() << "/" << size() << "] "
#define DEBUG_CERR vistle::DoNotPrintInstance


LibSimModule::LibSimModule(const string& name, int moduleID, mpi::communicator comm)
    : InSituReader("View and controll optins for LibSim instrumented simulations", name, moduleID, comm)
#if BOOST_VERSION >= 106600
    , m_workGuard(boost::asio::make_work_guard(m_ioService))
#else
    , m_workGuard(new boost::asio::io_service::work(m_ioService))
#endif
    , m_ioThread([this]() { 
        m_ioService.run();
        DEBUG_CERR << "io thread terminated" << endl;
        })
, m_socketComm(comm, boost::mpi::comm_create_kind::comm_duplicate) {

    m_acceptorv4.reset(new boost::asio::ip::tcp::acceptor(m_ioService));
    m_acceptorv6.reset(new boost::asio::ip::tcp::acceptor(m_ioService));

    m_filePath = addStringParameter("file Path", "path to a .sim2 file", "", vistle::Parameter::ExistingFilename);
    setParameterFilters(m_filePath, "Simulation Files (*.sim2)");
    m_simName = addStringParameter("Simulation name", "the name of the simulation as used in the filename of the sim2 file ", "");
    


    m_intOptions[InSituMessageType::VTKVariables] = std::unique_ptr< IntParam<insitu::message::VTKVariables>>(new IntParam<insitu::message::VTKVariables>{ addIntParameter("VTKVariables", "sort the variable data on the grid from VTK ordering to Vistles", false, vistle::Parameter::Boolean) });
    m_intOptions[InSituMessageType::ConstGrids] = std::unique_ptr< IntParam<insitu::message::ConstGrids>>(new IntParam<insitu::message::ConstGrids>{ addIntParameter("contant grids", "are the grids the same for every timestep?", false, vistle::Parameter::Boolean) });
    m_intOptions[InSituMessageType::NthTimestep] = std::unique_ptr< IntParam<insitu::message::NthTimestep>>(new IntParam<insitu::message::NthTimestep>{ addIntParameter("frequency", "frequency in whic data is retrieved from the simulation", 1) });
    m_intOptions[InSituMessageType::CombineGrids] = std::unique_ptr< IntParam<insitu::message::CombineGrids>>(new IntParam<insitu::message::CombineGrids>{ addIntParameter("Combine grids", "combine all structure grids on a rank to a single unstructured grid", false, vistle::Parameter::Boolean) });
    m_intOptions[InSituMessageType::KeepTimesteps] = std::unique_ptr< IntParam<insitu::message::KeepTimesteps>>(new IntParam<insitu::message::KeepTimesteps>{ addIntParameter("keep timesteps", "keep data of processed timestep of this execution", true, vistle::Parameter::Boolean) });

    std::string mqName = vistle::message::MessageQueue::createName("recvFromSim", moduleID, rank());
    try {
        m_receiveFromSimMessageQueue.reset(vistle::message::MessageQueue::create(mqName));
        DEBUG_CERR << "sendMessageQueue name = " << mqName << std::endl;
    } catch (boost::interprocess::interprocess_exception & ex) {
        throw vistle::exception(std::string("opening send message queue ") + mqName + ": " + ex.what()); 
    }

    if (rank() == 0) {
        startControllServer();

    } else {
        startSocketThread();
    }
}

LibSimModule::~LibSimModule() {
    setBool(m_terminateSocketThread, true);
    comm().barrier(); //make sure m_terminate is set on all ranks before releasing slaves from waiting for connection
    if (rank() == 0) {
        boost::system::error_code ec;
        if (getBool(m_connectedToEngine)) {
            m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive); //release me from waiting for messages
            InSituTcpMessage::send(insitu::message::ConnectionClosed{});
            m_socket->close(ec);
        } else {
            boost::system::error_code ec;
            m_ListeningSocket->cancel(ec);// release me from waiting for connection
            m_socketComm.barrier(); // release slaves from waiting for connection
        }
    }
    if (m_socketThread.joinable()) {
        m_socketThread.join();
    }
    m_workGuard.reset();
    m_ioService.stop();
    m_ioThread.join();
}

bool LibSimModule::prepareReduce() {

    InSituTcpMessage::send(insitu::message::Ready{ false });
    if (m_connectedToEngine) {
        bool r = false;
        {
            SyncShmMessage msg = SyncShmMessage::timedRecv(2, r);
            if (r && (vistle::Shm::the().objectID() != msg.objectID() || vistle::Shm::the().arrayID() != msg.arrayID())) {
                CERR << "permanently sending shm ids does not work!!!!!!!!!" << endl;
                vistle::Shm::the().setObjectID(msg.objectID());
                vistle::Shm::the().setArrayID(msg.arrayID());
            }
        }
        bool result = false;
        auto f = [](bool a, bool b) { return false; };
        boost::mpi::all_reduce(comm(), r, result, mpi::minimum<bool>());
        if (!result) { //closeing tcp connection will restart the connection process
            m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive); //release me from waiting for messages
            m_socket->close();
        }
    }
    return true;
}

bool LibSimModule::prepare() {

    if (!getBool(m_connectedToEngine)) {
        return true;
    }
    vector<string> connectedPorts;
    for (const auto port : m_outputPorts)         {
        if (port.second->isConnected()) {
            connectedPorts.push_back(port.first);
        }
    }
    InSituTcpMessage::send(insitu::message::AddPorts{ connectedPorts });
    InSituTcpMessage::send(insitu::message::Ready{ true });
    SyncShmMessage::send(SyncShmMessage{ vistle::Shm::the().objectID(), vistle::Shm::the().arrayID() });

    return true;
}

bool LibSimModule::dispatch(bool block, bool* messageReceived) {
    vistle::message::Buffer buf;
    bool passMsg = false;
    if (m_receiveFromSimMessageQueue->tryReceive(buf)) {
        if (buf.type() != vistle::message::INSITU) {
            sendMessage(buf);
        }
        passMsg = true;
    }
    bool msgRecv;
    auto retval =  Module::dispatch(false, &msgRecv);
    vistle::adaptive_wait((msgRecv || passMsg));
    if (messageReceived) {
        *messageReceived = msgRecv;
    }
    return retval;


}

bool LibSimModule::changeParameter(const vistle::Parameter* param) {
    Module::changeParameter(param);
    if (!param) {
        CERR << "called without param " << m_filePath->getValue() << endl;
        return true;
    }
    if (param == m_filePath || param == m_simName) {
        connectToSim();
    } else if (m_commandParameter.find(param) != m_commandParameter.end()) {

        InSituTcpMessage::send(insitu::message::ExecuteCommand(param->getName()));
    } else {
        for (const auto &option : m_intOptions) {
            if (option.second->param() == param) {
                option.second->send();
                continue;
            }
        }
    }

    return InSituReader::changeParameter(param);
}

void LibSimModule::startControllServer() {
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
    startAccept(m_acceptorv4);
    startAccept(m_acceptorv6);

    return;
}

bool LibSimModule::startAccept(shared_ptr<acceptor> a) {

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
        if (m_socketThread.joinable()) {
            m_socketThread.join();
        }
        startSocketThread();
        });
    return true;
}

void LibSimModule::startSocketThread()     {
    SyncShmMessage::initialize(id(), rank(), SyncShmMessage::Mode::Create);
    m_socketThread = std::thread([this]() {
        m_socketComm.barrier();
        setBool(m_connectedToEngine, true);
        InSituTcpMessage::initialize(m_socket, m_socketComm);
        for (const auto &option : m_intOptions)             {
            option.second->send();
        }
        while (!getBool(m_terminateSocketThread)) {
            bool r = false;
            {
                do {
                    SyncShmMessage msg = SyncShmMessage::tryRecv(r);
                    if (r) {
                        vistle::Shm::the().setObjectID(msg.objectID());
                        vistle::Shm::the().setArrayID(msg.arrayID());
                    }
                } while (r);
            }
            recvAndhandleMessage();
        }
        });

}

void LibSimModule::recvAndhandleMessage()     {

    InSituTcpMessage msg = InSituTcpMessage::recv();
    
    DEBUG_CERR << "handleMessage " << (int)msg.type() << endl;
    using namespace insitu;
    switch (msg.type()) {
    case InSituMessageType::Invalid:
        break;
    case InSituMessageType::ShmInit:
        break;
    case InSituMessageType::AddObject:
        break;
    case InSituMessageType::AddPorts:
    {
        auto em = msg.unpackOrCast< message::AddPorts>();
        for (size_t i = 0; i < em.value.size() - 1; i++) {
            createOutputPort(em.value[i], em.value[em.value.size() - 1]);
        }
    }
        break;
    case InSituMessageType::AddCommands:
    {
        auto em = msg.unpackOrCast< message::AddCommands>();
        for (size_t i = 0; i < em.value.size(); i++) {
            m_commandParameter.insert(addIntParameter(em.value[i], "", false, vistle::Parameter::Presentation::Boolean));
        }
    }
        break;
    case InSituMessageType::Ready:
        break;
    case InSituMessageType::ExecuteCommand:
        break;
    case InSituMessageType::GoOn:
    {
        InSituTcpMessage::send(message::GoOn{});
    }
        break;
    case InSituMessageType::ConstGrids:
        break;
    case InSituMessageType::NthTimestep:
        break;
    case InSituMessageType::ConnectionClosed:
    {
        CERR << " tcp connection closed...disconnecting." << endl;
        disconnectSim();
    }
        break;
    default:
        break;
    }
}

void LibSimModule::connectToSim()     {

    if (m_simInitSent) {
        DEBUG_CERR << "already connected" << endl;
    } else if (rank() == 0) {
        using namespace boost::filesystem;
        path p{ m_filePath->getValue() };
        if (is_directory(p)) {
            auto simName = m_simName->getValue();
            path lastEditedFile;
            std::time_t lastEdit{};
            for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(p), {})) {
                if (simName.size() == 0 || entry.path().filename().generic_string().find(simName + ".sim2") != std::string::npos) {
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
        vector<string> args{ to_string(size()), vistle::Shm::the().name(), name(), to_string(id()), vistle::hostname(), to_string(m_port) };
        if (insitu::attemptLibSImConnection(p.string(), args)) {
            m_simInitSent = true;
        }
    }
    boost::mpi::broadcast(comm(), m_simInitSent, 0);
}

void LibSimModule::disconnectSim()     {
    {
        Guard g(m_socketMutex);
        m_simInitSent = false;
        m_connectedToEngine = false;
        if (m_terminateSocketThread) {
            return;
        }
    }
    if (rank() == 0) {
        CERR << "conection closed, listening for connections on port " << m_port << endl;
        startAccept(m_acceptorv4);
        startAccept(m_acceptorv6);
        return;
    } else {
        m_socketComm.barrier(); //wait for rank 0 to reconnect
        InSituTcpMessage::initialize(m_socket, m_socketComm);
    }
}

void LibSimModule::setBool(bool& target, bool newval) {
    Guard g(m_socketMutex);
    target = newval;
}

bool LibSimModule::getBool(const bool &val) {
    Guard g(m_socketMutex);
    return val;
}




MODULE_MAIN_THREAD(LibSimModule, MPI_THREAD_MULTIPLE)
