#include "LibSimControlModule.h"
#include <util/sleep.h>
#include <util/listenv4v6.h>
#include <util/hostname.h>

#include <insitu/LibSim/EstablishConnection.h>

#include <core/rectilineargrid.h>
#include <sstream>

#include <boost/bind.hpp>

#include "EngineMessage.h"
#include <core/message.h>
#include <core/tcpmessage.h>

using namespace std;
using in_situ::EngineMessage;

#define CERR cerr << "["<< rank() << "/" << size() << "] "

ControllModule::ControllModule(const string& name, int moduleID, mpi::communicator comm)
    : vistle::InSituReader("set ConnectLibSim in the state of prepare", name, moduleID, comm) 
    , m_acceptorv4(new boost::asio::ip::tcp::acceptor(m_ioService))
    , m_acceptorv6(new boost::asio::ip::tcp::acceptor(m_ioService))
#if BOOST_VERSION >= 106600
    , m_workGuard(boost::asio::make_work_guard(m_ioService))
#else
    , m_workGuard(new boost::asio::io_service::work(m_ioService))
#endif
    , m_ioThread([this]() { m_ioService.run(); })
    , m_socketComm(comm, boost::mpi::comm_create_kind::comm_duplicate)
{
    noDataOut = createOutputPort("noData", "");
    m_filePath = addStringParameter("file Path", "path to a .sim2 file", "", vistle::Parameter::ExistingFilename);
    setParameterFilters(m_filePath, "Simulation Files (*.sim2)");
    //setParameterFilters(m_resultfiledir, "Result Files (*.res)/All Files (*)");
    sendCommand = addIntParameter("sendCommand", "send the command to the simulation", false, vistle::Parameter::Boolean);
    m_constGrids = addIntParameter("contant grids", "are the grids the same for every timestep?", false, vistle::Parameter::Boolean);
    m_nthTimestep = addIntParameter("frequency", "frequency in whic data is retrieved from the simulation", 1);
    setParameterMinimum(m_nthTimestep, static_cast<vistle::Integer>(1));
    sendMessageToSim = addIntParameter("sendMessageToSim", "", false, vistle::Parameter::Boolean);
    observeParameter(sendMessageToSim);
    observeParameter(m_filePath);
    observeParameter(sendCommand);
    observeParameter(m_constGrids);
    observeParameter(m_nthTimestep);
    if (rank() == 0) {
        startControllServer();

    } else {
        m_socketThread = std::thread([this]() {
            m_socketComm.barrier();
            in_situ::EngineMessage::initializeEngineMessage(m_socket, m_socketComm);
            waitForMessages();
            });
    }


}

ControllModule::~ControllModule() {
    m_workGuard.reset();
    m_ioService.stop();
    m_ioThread.join();
}

void ControllModule::startControllServer() {
    unsigned short port = m_port;

    boost::system::error_code ec;

    while (!vistle::start_listen(port, *m_acceptorv4, *m_acceptorv6, ec)) {
        if (ec == boost::system::errc::address_in_use) {
            ++port;
        } else if (ec) {
            cerr << "failed to listen on port " << port << ": " << ec.message() << endl;
            return;
        }
    }

    m_port = port;

    cerr << "listening for connections on port " << m_port << endl;
    startAccept(m_acceptorv4);
    startAccept(m_acceptorv4);

    return;
}

bool ControllModule::startAccept(shared_ptr<acceptor> a) {

    if (!a->is_open())
        return false;

    shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(m_ioService));
    m_ListeningSocket = sock;
    boost::system::error_code ec;
    a->async_accept(*sock, [this, a, sock](boost::system::error_code ec) {
        if (ec) {
            m_socket = nullptr;
            cerr << "failed connection attempt" << endl;
            return;
        }
        cerr << "connected with engine" << endl;
        m_socket = sock;
        m_socketComm.barrier();
        m_socketThread = std::thread([this]() {
            in_situ::EngineMessage::initializeEngineMessage(m_socket, m_socketComm);
            waitForMessages();
            });
        });
    return true;
}

bool ControllModule::prepare() {

    EngineMessage::sendEngineMessage(in_situ::EM_Ready(true));
    return true;
}

bool ControllModule::reduce(int timestep) {
    EngineMessage::sendEngineMessage(in_situ::EM_Ready(false));
    m_timestep = 0;
    return true;
}

bool ControllModule::examine(const vistle::Parameter* param) {
    if (!param) {
        return true;
    }
    if (param == sendCommand && isExecuting()) {
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
        addObject(noDataOut, variable);
        ++m_timestep;
    }
    else if (rank() != 0) {
        return true;
    }
    if (param == sendMessageToSim) {
        EngineMessage::sendEngineMessage(in_situ::EM_GoOn(true));
    }
    else if (m_commandParameter.find(param) != m_commandParameter.end()) {
        EngineMessage::sendEngineMessage(in_situ::EM_ExecuteCommand(param->getName()));
    }
    else if (param == m_filePath) {
        cerr << "test" << endl;
        vector<string> args{to_string(size()), "shm", name(), to_string(id()), vistle::hostname(), to_string(m_port) };
        in_situ::attemptLibSImConnection(m_filePath->getValue(), args);
    }
    else if(param == m_constGrids){
        EngineMessage::sendEngineMessage(in_situ::EM_ConstGrids(m_constGrids->getValue()));
    }
    else if (param == m_nthTimestep) {
        EngineMessage::sendEngineMessage(in_situ::EM_NthTimestep (m_nthTimestep->getValue()) );
    }

    return false;
}



void ControllModule::handleMessage(const in_situ::EngineMessage& msg)     {
    
    //toDO
    waitForMessages();

}

void ControllModule::waitForMessages()     {
    while (true) {
       
        handleMessage(EngineMessage::recvEngineMessage());
    }
}


MODULE_MAIN(ControllModule)
