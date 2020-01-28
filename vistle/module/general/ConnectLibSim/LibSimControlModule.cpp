#include "LibSimControlModule.h"
#include <util/sleep.h>
#include <util/listenv4v6.h>
#include <util/hostname.h>

#include <insitu/LibSim/EstablishConnection.h>

#include <core/rectilineargrid.h>


#include <boost/bind.hpp>

using namespace std;

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
{
    noDataOut = createOutputPort("noData", "");
    m_filePath = addStringParameter("file Path", "path to a .sim2 file", "");
    sendCommand = addIntParameter("sendCommand", "send the command to the simulation", false, vistle::Parameter::Boolean);
    observeParameter(m_filePath);
    observeParameter(sendCommand);
    if (rank() == 0) {
        startControllServer();
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
    startAccept(m_acceptorv6);

    return;
}

bool ControllModule::startAccept(shared_ptr<acceptor> a) {

    if (!a->is_open())
        return false;

    shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(m_ioService));
    m_ListeningSocket = sock;
    cerr << " acceptor is open" << endl;
    a->async_accept(*sock, [this, a, sock](boost::system::error_code ec) {
        if (ec) {
            m_socket = nullptr;
            cerr << "failed connection attempt" << endl;
            return;
        }
        cerr << "connected!" << endl;
        m_socket = sock;
        waitForMessage();
        startAccept(a);
        });

    return true;
}

bool ControllModule::prepare() {
    cerr << "prepared" << endl;
    return true;
}

bool ControllModule::reduce(int timestep) {
    cerr << "reduced" << endl;
    m_timestep = 0;
    return true;
}

bool ControllModule::examine(const vistle::Parameter* param) {
    if (!param) {
        return true;
    }

    if (param == m_filePath) {
        vector<string> args{to_string(size()), "shm", name(), to_string(id()), vistle::hostname(), to_string(m_port) };
        in_situ::attemptLibSImConnection(m_filePath->getValue(), args);
    }
    else if (param == sendCommand && isExecuting()) {
        array<int, 3> dims{ 5, 10, 3 };
        vistle::RectilinearGrid::ptr grid = vistle::RectilinearGrid::ptr(new vistle::RectilinearGrid(dims[0], dims[1], dims[2]));
        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < dims[i]; j++) {
                grid->coords(i)[j] = j;
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

    return false;
}

void ControllModule::sendMsgToSim(const std::string& msg)
{
    boost::system::error_code ec;
    boost::asio::write(*m_socket, boost::asio::buffer(msg), ec);
    if (ec) {
        std::cerr << "failed to send message key to sim" << std::endl;
    }
}

void ControllModule::handleMessage(const boost::system::error_code& err, size_t bytes_transferred)     {
    
    if (err) {
        cerr << "failed to read from engine socket" << endl;
        return;
    }
    string msg;
    boost::system::error_code ec;
    m_streambuf.commit(bytes_transferred);
    std::istream is(&m_streambuf);
    is >> msg;
    
    cerr << "received message " << msg << endl;
    waitForMessage();
}

void ControllModule::waitForMessage()     {
    boost::asio::async_read_until(*m_socket, m_streambuf, '\n',
        boost::bind(&ControllModule::handleMessage, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}
MODULE_MAIN(ControllModule)
