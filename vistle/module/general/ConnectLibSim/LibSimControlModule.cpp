#include "LibSimControlModule.h"
#include <util/sleep.h>
#include <util/listenv4v6.h>
#include <util/hostname.h>

#include <insitu/LibSim/EstablishConnection.h>

#include <core/rectilineargrid.h>
#include <sstream>

#include <boost/bind.hpp>

#include "EngineMessage.h"

using namespace std;
using in_situ::EngineMessage;
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
    }
    //test
    boost::asio::streambuf b;
    std::ostream os(&b);
    os << "abc";
    os << 5;
    os << std::string("def");
    b.commit(b.size());
    std::istream is(&b);
    std::string a, d;
    a.resize(3);
    is.read(a.data(), 3);
    
    int i;
    is >> i >> d;
    std::cerr << " a= " << a << ", i = " << i << ", d = " << d << std::endl;
    //endtest

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
        });

    return true;
}

bool ControllModule::prepare() {

    EngineMessage msg(EngineMessage::ready);
    msg << true;
    msg.sendMessage(m_socket); 
    return true;
}

bool ControllModule::reduce(int timestep) {
    EngineMessage msg(EngineMessage::ready);
    msg << false;
    msg.sendMessage(m_socket);
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
        in_situ::EngineMessage msg{ in_situ::EngineMessage::goOn };
        msg.sendMessage(m_socket);
    }
    else if (m_commandParameter.find(param) != m_commandParameter.end()) {
        in_situ::EngineMessage msg(in_situ::EngineMessage::executeCommand);
        msg << param->getName();
        msg.sendMessage(m_socket);
    }
    else if (param == m_filePath) {
        cerr << "test" << endl;
        vector<string> args{to_string(size()), "shm", name(), to_string(id()), vistle::hostname(), to_string(m_port) };
        in_situ::attemptLibSImConnection(m_filePath->getValue(), args);
    }
    else if(param == m_constGrids){
        EngineMessage msg{ EngineMessage::constGrids };
        msg << true;
        msg.sendMessage(m_socket);
    }
    else if (param == m_nthTimestep) {
        EngineMessage msg{EngineMessage::nThTimestep};
        msg << true;
        msg.sendMessage(m_socket);
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

void ControllModule::handleMessage(in_situ::EngineMessage&& msg)     {
    
    switch (msg.type()) {
    case in_situ::EngineMessage::invalid:
        break;
    case in_situ::EngineMessage::shmInit:
        break;
    case in_situ::EngineMessage::addObject:
        break;
    case in_situ::EngineMessage::addPorts:
    {
        string type; 
        vector<string> elements;
        msg >> type;
        msg >> elements;
        for (auto element : elements) {
            m_outputPorts[element] = createOutputPort(element, type);
        }
    }
        break;
    case in_situ::EngineMessage::addCommands:
    {
        std::vector<std::string> commands;
        msg >> commands;
        for (auto command : commands)             {
            auto c = addIntParameter(command, command, vistle::Parameter::Boolean);
            m_commandParameter.insert(c);
            observeParameter(c);
        }
    }
        break;
    case in_situ::EngineMessage::executeCommand:
        break;
    case in_situ::EngineMessage::goOn:
    {
        EngineMessage msg(EngineMessage::goOn);
        msg.sendMessage(m_socket);
    }
        break;
    default:
        break;
    }
    waitForMessage();

}

void ControllModule::waitForMessage()     {
    in_situ::EngineMessage::read(m_socket, [this](in_situ::EngineMessage&& msg) {
        handleMessage(std::move(msg));
        });
}
MODULE_MAIN(ControllModule)
