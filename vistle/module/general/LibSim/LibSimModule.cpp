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

using namespace std;
using insitu::message::InSituTcpMessage;
using insitu::message::SyncShmMessage;
using insitu::message::InSituMessageType;

#define CERR cerr << "LibSimModule["<< rank() << "/" << size() << "] "



LibSimModule::LibSimModule(const string& name, int moduleID, mpi::communicator comm)
    : InSituReader("View and controll optins for LibSim instrumented simulations", name, moduleID, comm)
#if BOOST_VERSION >= 106600
    , m_workGuard(boost::asio::make_work_guard(m_ioService))
#else
    , m_workGuard(new boost::asio::io_service::work(m_ioService))
#endif
    , m_ioThread([this]() { m_ioService.run();
CERR << "io thread terminated" << endl; })
, m_socketComm(comm, boost::mpi::comm_create_kind::comm_duplicate) {

    m_acceptorv4.reset(new boost::asio::ip::tcp::acceptor(m_ioService));
    m_acceptorv6.reset(new boost::asio::ip::tcp::acceptor(m_ioService));

    m_filePath = addStringParameter("file Path", "path to a .sim2 file", "", vistle::Parameter::ExistingFilename);
    setParameterFilters(m_filePath, "Simulation Files (*.sim2)");
    //setParameterFilters(m_resultfiledir, "Result Files (*.res)/All Files (*)");
    sendCommand = addIntParameter("sendCommand", "send the command to the simulation", false, vistle::Parameter::Boolean);
    m_constGrids = addIntParameter("contant grids", "are the grids the same for every timestep?", false, vistle::Parameter::Boolean);
    m_nthTimestep = addIntParameter("frequency", "frequency in whic data is retrieved from the simulation", 1);
    setParameterMinimum(m_nthTimestep, static_cast<vistle::Integer>(1));
    sendMessageToSim = addIntParameter("sendMessageToSim", "", false, vistle::Parameter::Boolean);

    std::string mqName = vistle::message::MessageQueue::createName("recvFromSim", moduleID, rank());
    try {
        m_receiveFromSimMessageQueue.reset(vistle::message::MessageQueue::create(mqName));
        std::cerr << "sendMessageQueue name = " << mqName << std::endl;
    } catch (boost::interprocess::interprocess_exception & ex) {
        throw vistle::exception(std::string("opening send message queue ") + mqName + ": " + ex.what());
    }


    SyncShmMessage::initialize(id(), rank(), SyncShmMessage::Mode::Create);
    if (rank() == 0) {
        startControllServer();

    } else {
        startSocketThread();
    }
}

LibSimModule::~LibSimModule() {
    setBool(m_terminate, true);
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
    if (getBool(m_connectedToEngine)) {
        SyncShmMessage msg = SyncShmMessage::recv();
        vistle::Shm::the().setObjectID(msg.objectID());
        vistle::Shm::the().setArrayID(msg.arrayID());
    }
    m_timestep = 0;
    return true;
}

bool LibSimModule::prepare() {
    InSituTcpMessage::send(insitu::message::Ready{ true });
    if (m_connectedToEngine) {
        SyncShmMessage::send(SyncShmMessage{ vistle::Shm::the().objectID(), vistle::Shm::the().arrayID() });
    }
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
        return true;
    }
    if (param == m_filePath) {
        if (m_simInitSent) {
            CERR << "already connected" << endl;
        } else if (rank() == 0) {
            vector<string> args{ to_string(size()), vistle::Shm::the().name(), name(), to_string(id()), vistle::hostname(), to_string(m_port) };
            if (insitu::attemptLibSImConnection(m_filePath->getValue(), args)) {
                m_simInitSent = true;
            }
        }
        boost::mpi::broadcast(comm(), m_simInitSent, 0);
    } else if (param == sendCommand && isExecuting()) {
        array<int, 3> dims{ 5, 10, 3 };
        vistle::RectilinearGrid::ptr grid = vistle::RectilinearGrid::ptr(new vistle::RectilinearGrid(dims[0], dims[1], dims[2]));
        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < dims[i]; j++) {
                grid->coords(i)[j] = j * (1 + rank());
            }
        }
        int nTuples = dims[0] * dims[1] * dims[2];
        vistle::Vec<vistle::Scalar>::ptr variable(new typename vistle::Vec<vistle::Scalar>(nTuples));
        for (size_t i = 0; i < nTuples; i++) {
            variable->x().data()[i] = i * m_timestep;
        }
        variable->setGrid(grid);
        variable->setTimestep(m_timestep);
        variable->setMapping(vistle::DataBase::Vertex);
        variable->addAttribute("_species", "velocity");
        ++m_timestep;
    } else if (rank() != 0) {
        return true;
    } else if (param == sendMessageToSim) {
        InSituTcpMessage::send(insitu::message::GoOn());
    } else if (m_commandParameter.find(param) != m_commandParameter.end()) {
        InSituTcpMessage::send(insitu::message::ExecuteCommand(param->getName()));
    } else if (param == m_constGrids) {
        InSituTcpMessage::send(insitu::message::ConstGrids(m_constGrids->getValue()));
    } else if (param == m_nthTimestep) {
        InSituTcpMessage::send(insitu::message::NthTimestep(m_nthTimestep->getValue()));
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
    m_socketThread = std::thread([this]() {
        m_socketComm.barrier();
        setBool(m_connectedToEngine, true);
        InSituTcpMessage::initialize(m_socket, m_socketComm);
        InSituTcpMessage::send(insitu::message::ConstGrids(m_constGrids->getValue()));
        InSituTcpMessage::send(insitu::message::NthTimestep(m_nthTimestep->getValue()));
        while (!getBool(m_terminate)) {
            recvAndhandleMessage();
        }
        });

}

void LibSimModule::recvAndhandleMessage()     {

    InSituTcpMessage msg = InSituTcpMessage::recv();
    
    CERR << "handleMessage " << (int)msg.type() << endl;
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
        for (size_t i = 0; i < em.m_portList.size() - 1; i++) {
            createOutputPort(em.m_portList[i], em.m_portList[em.m_portList.size() - 1]);
        }
    }
        break;
    case InSituMessageType::AddCommands:
    {
        auto em = msg.unpackOrCast< message::AddCommands>();
        for (size_t i = 0; i < em.m_commandList.size(); i++) {
            m_commandParameter.insert(addIntParameter(em.m_commandList[i], "", false, vistle::Parameter::Presentation::Boolean));
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
        {
            Guard g(m_socketMutex);
            m_simInitSent = false;
            m_connectedToEngine = false;
            if (m_terminate) {
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
        break;
    default:
        break;
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
