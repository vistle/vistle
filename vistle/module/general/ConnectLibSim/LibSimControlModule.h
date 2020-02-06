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

    typedef std::lock_guard<std::mutex> Guard;

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
    bool m_terminate = false; //set to true when when the module in closed to get out of loops in different threads
    bool m_simInitSent = false; //to prevent caling attemptLibSImConnection twice
    bool m_connectedToEngine = false; //wether the socket connection to the engine is running
    std::map<std::string, vistle::Port*> m_outputPorts; //output ports for the data the simulation offers
    std::set<const vistle::Parameter*> m_commandParameter;

    //.........................................................................
    //stuff to handle socket communication with Engine
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

    std::mutex m_socketMutex;
    //..........................................................................


        //..........................................................................                                               
    //module functions
    virtual bool prepare() override;
    virtual bool reduce(int timestep) override;
    virtual bool examine(const vistle::Parameter* param);
    //..........................................................................



    void startControllServer(); //find a free socket to listen to and start accept

    bool startAccept(std::shared_ptr<acceptor> a); //async accept initiates handle message toop
    
    void startSocketThread();
    
    void recvAndhandleMessage();

    //..........................................................................
    //thread safe (for the socket thread) getter and setter for bools
    void setBool(bool & target, bool newval);

    bool getBool(const bool& val);


    //test
    int m_timestep = 0;

};


#endif // !LIBSIM_CONTROL_MODULE_H
