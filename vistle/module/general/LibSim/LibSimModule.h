#ifndef LIBSIM_CONTROL_MODULE_H
#define LIBSIM_CONTROL_MODULE_H

#include <vistle/insitu/libsim/IntOption.h>
#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/insitu/message/TcpMessage.h>
#include <vistle/insitu/module/inSituReader.h>

#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
class LibSimModule: public vistle::insitu::InSituReader {
public:
    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;

    typedef std::lock_guard<std::mutex> Guard;

    LibSimModule(const std::string &name, int moduleID, mpi::communicator comm);
    ~LibSimModule();

private:
    virtual bool beginExecute() override;
    virtual bool endExecute() override;
    virtual bool changeParameter(const vistle::Parameter *param);

    void startControllServer();
    bool startAccept(std::shared_ptr<acceptor> a); // async accept initiates handle message toop
    void startSocketThread();
    void resetSocketThread(); // blocks rank 0 and slaves until rank 0 is connected
    void recvAndhandleMessage();
    void connectToSim();
    void disconnectSim();

    //..........................................................................
    // thread safe (for the socket thread) getter and setter for bools
    void setBool(bool &target, bool newval);
    bool getBool(const bool &val);

    vistle::insitu::message::InSituTcp m_messageHandler;
#ifndef MODULE_THREAD
    vistle::StringParameter *m_filePath = nullptr;
    vistle::StringParameter *m_simName = nullptr;
#endif
    bool m_terminateSocketThread =
        false; // set to true when when the module in closed to get out of loops in different threads
    bool m_simInitSent = false; // to prevent calling attemptLibSImConnection twice
    bool m_connectedToEngine = false; // whether the socket connection to the engine is running
    bool m_firstConnectionAttempt = true;
    std::map<std::string, vistle::Port *> m_outputPorts; // output ports for the data the simulation offers
    std::set<vistle::Parameter *> m_commandParameter; // buttons to trigger simulation commands
    std::set<vistle::Parameter *> m_customCommandParameter; // string inputs to trigger simulation commands

    std::thread m_socketThread; // thread to sync tcp communication with slaves
    boost::mpi::communicator m_socketComm; // communicator to sync tcp communication with slaves
    std::mutex m_socketMutex; // mutex to sync main and socket thread
#ifndef MODULE_THREAD
    //.........................................................................
    // stuff to handle socket communication with Engine
    unsigned short m_port = 31299;
    boost::asio::io_service m_ioService;
    std::shared_ptr<acceptor> m_acceptorv4, m_acceptorv6;
    std::shared_ptr<boost::asio::ip::tcp::socket> m_socket, m_ListeningSocket;

#if BOOST_VERSION >= 106600
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;
#else
    std::shared_ptr<boost::asio::io_service::work> m_workGuard;
#endif

    std::thread m_ioThread; // thread for io_service

    std::mutex m_asioMutex; // to let the rank 0 socket thread wait for connection (the slaves use mpi barrier);
    std::condition_variable
        m_connectedCondition; // to let the rank 0 socket thread wait for connection in the asio thread
    bool m_waitingForAccept = false; // condition
#endif

    std::map<const vistle::IntParameter *, vistle::insitu::libsim::IntOptions> m_intOptions;
};

#endif // !LIBSIM_CONTROL_MODULE_H
