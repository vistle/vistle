#ifndef LIBSIM_CONTROL_MODULE_H
#define LIBSIM_CONTROL_MODULE_H
#include <module/inSituReader.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>

#include <core/message.h>
#include "EngineMessage.h"

class ControllModule : public vistle::InSituReader
{
public:

    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;

    ControllModule(const std::string& name, int moduleID, mpi::communicator comm);
    ~ControllModule();
    vistle::Port* noDataOut = nullptr;
    vistle::StringParameter* m_filePath = nullptr;
    vistle::IntParameter* m_constGrids = nullptr;
    vistle::IntParameter* m_nthTimestep = nullptr;
    vistle::IntParameter* sendMessageToSim = nullptr;
    vistle::IntParameter* sendCommand = nullptr;
    bool sendCommandChanged = false;
private:

    std::map<std::string, vistle::Port*> m_outputPorts;
    std::set<const vistle::Parameter*> m_commandParameter;

    unsigned short m_port = 31299;
    boost::asio::io_service m_ioService;
    std::shared_ptr<acceptor> m_acceptorv4, m_acceptorv6;
    std::shared_ptr<boost::asio::ip::tcp::socket> m_socket, m_ListeningSocket;
    boost::asio::streambuf m_streambuf;

#if BOOST_VERSION >= 106600
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;
#else
    std::shared_ptr<boost::asio::io_service::work> m_workGuard;
#endif
    //thread for io_service
    std::thread m_ioThread;
    //thread to sync tcp communcation with slaves
    std::thread m_socketThread;
    //communicator to sync tcp communcation with slaves
    boost::mpi::communicator m_socketComm;

    void startControllServer();

    bool startAccept(std::shared_ptr<acceptor> a);

    virtual bool prepare() override; 
    virtual bool reduce(int timestep) override; 
    virtual bool examine(const vistle::Parameter* param);

    void sendMsgToSim(const std::string& msg);


    void handleMessage(insitu::EngineMessage&& msg);

    void waitForMessages();


    //test
    int m_timestep = 0;

};


#endif // !LIBSIM_CONTROL_MODULE_H
