#ifndef LIBSIM_CONTROL_MODULE_H
#define LIBSIM_CONTROL_MODULE_H
#include <insitu/module/inSituReader.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>

#include <core/message.h>
#include <core/messagequeue.h>

#include <insitu/message/SyncShmIDs.h>
#include <insitu/message/TcpMessage.h>

class LibSimModule : public insitu::InSituReader
{
public:

    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;

    typedef std::lock_guard<std::mutex> Guard;

    LibSimModule(const std::string& name, int moduleID, mpi::communicator comm);
    ~LibSimModule();

    

private:
    insitu::message::InSituTcp m_messageHandler;
    insitu::message::SyncShmIDs m_shmIDs;
#ifndef MODULE_THREAD
    vistle::StringParameter* m_filePath = nullptr;
    vistle::StringParameter* m_simName = nullptr;
#endif
    bool m_terminateSocketThread = false; //set to true when when the module in closed to get out of loops in different threads
    bool m_simInitSent = false; //to prevent caling attemptLibSImConnection twice
    bool m_connectedToEngine = false; //wether the socket connection to the engine is running
    bool m_firstConnectionAttempt = true;
    int m_numberOfConnections = 0; //number of times we have been connected with a simulation, used to rename reopend shm queues.
    std::unique_ptr<vistle::message::MessageQueue> m_receiveFromSimMessageQueue; //receives vistle messages that will be passed through to manager
    std::map<std::string, vistle::Port*> m_outputPorts; //output ports for the data the simulation offers
    std::set<vistle::Parameter*> m_commandParameter; //buttons to trigger simulation commands
    //...................................................................................
    //used to manag the int and bool options this module always offers
    struct IntParamBase {
        IntParamBase() {}

        virtual void send() { return; };
        const vistle::IntParameter* param() const {
            return m_param;
        }
    protected:
        IntParamBase(vistle::IntParameter* param)
            :m_param(param) {
        }
    private:
        vistle::IntParameter* m_param = nullptr;
    };

    template<typename T>
    struct IntParam : public IntParamBase {
        IntParam(vistle::IntParameter* param, const insitu::message::InSituTcp& sender)
            :IntParamBase(param) 
        , m_sender(sender){
        }
        const insitu::message::InSituTcp& m_sender;
        virtual void send() override {
            m_sender.send(T{ static_cast<typename T::value_type>(param()->getValue()) });
        }
    };
    std::map<insitu::message::InSituMessageType, std::unique_ptr<IntParamBase>> m_intOptions;
#ifndef MODULE_THREAD
    //.........................................................................
    //stuff to handle socket communication with Engine
    unsigned short m_port = 31299;
    boost::asio::io_service m_ioService;
    std::shared_ptr<acceptor> m_acceptorv4, m_acceptorv6;
    std::shared_ptr<boost::asio::ip::tcp::socket> m_socket, m_ListeningSocket;

#if BOOST_VERSION >= 106600
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;
#else
    std::shared_ptr<boost::asio::io_service::work> m_workGuard;
#endif
   
    std::thread m_ioThread; //thread for io_service
    
    std::mutex m_asioMutex; //to let the rank 0 socket thread wait for connection (the slaves use mpi barrier);
    std::condition_variable m_connectedCondition; //to let the rank 0 socket thread wait for connection in the asio thread
    bool m_waitingForAccept = false; //condition
#endif
    std::thread m_socketThread; //thread to sync tcp communcation with slaves

    boost::mpi::communicator m_socketComm;//communicator to sync tcp communcation with slaves

    std::mutex m_socketMutex; //mutex to sync main and socket thread
    //..........................................................................


        //..........................................................................                                               
    //module functions

    virtual bool prepare() override;
    virtual bool dispatch(bool block = true, bool* messageReceived = nullptr) override;
    virtual bool prepareReduce() override;
    virtual bool changeParameter(const vistle::Parameter* param);
    //..........................................................................


    void startControllServer();

    bool startAccept(std::shared_ptr<acceptor> a); //async accept initiates handle message toop
    
    void startSocketThread();

    void resetSocketThread(); //blocks rank 0 and slaves until rank 0 is connected
    
    void recvAndhandleMessage();

    void connectToSim();

    void disconnectSim();

    void initRecvFromSimQueue();

    //..........................................................................
    //thread safe (for the socket thread) getter and setter for bools
    void setBool(bool & target, bool newval);

    bool getBool(const bool& val);


};


#endif // !LIBSIM_CONTROL_MODULE_H
