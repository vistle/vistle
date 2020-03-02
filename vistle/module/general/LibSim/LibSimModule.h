#ifndef LIBSIM_CONTROL_MODULE_H
#define LIBSIM_CONTROL_MODULE_H
#include <insitu/module/inSituReader.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>

#include <core/message.h>
#include <core/messagequeue.h>

#include <insitu/message/InSituMessage.h>
class LibSimModule : public insitu::InSituReader
{
public:

    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;

    typedef std::lock_guard<std::mutex> Guard;

    LibSimModule(const std::string& name, int moduleID, mpi::communicator comm);
    ~LibSimModule();
    vistle::StringParameter* m_filePath = nullptr;
    vistle::StringParameter* m_simName = nullptr;
    
    struct IntParamBase
    { 
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
        IntParam(vistle::IntParameter* param)
            :IntParamBase(param){}

        virtual void send() override {
            insitu::message::InSituTcpMessage::send(T{ static_cast<typename T::value_type>(param()->getValue()) });
        }
    };
    std::map<insitu::message::InSituMessageType, std::unique_ptr<IntParamBase>> m_intOptions;
    bool sendCommandChanged = false;
private:
    bool m_terminateSocketThread = false; //set to true when when the module in closed to get out of loops in different threads
    bool m_simInitSent = false; //to prevent caling attemptLibSImConnection twice
    bool m_connectedToEngine = false; //wether the socket connection to the engine is running
    bool m_firstConnectionAttempt = true;
    std::map<std::string, vistle::Port*> m_outputPorts; //output ports for the data the simulation offers
    std::set<const vistle::Parameter*> m_commandParameter;
    std::unique_ptr<vistle::message::MessageQueue> m_receiveFromSimMessageQueue;
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
    virtual bool dispatch(bool block = true, bool* messageReceived = nullptr) override;
    virtual bool prepareReduce() override;
    virtual bool changeParameter(const vistle::Parameter* param);
    //..........................................................................



    void startControllServer(); //find a free socket to listen to and start accept

    bool startAccept(std::shared_ptr<acceptor> a); //async accept initiates handle message toop
    
    void startSocketThread();
    
    void recvAndhandleMessage();

    void connectToSim();

    void disconnectSim();

    //..........................................................................
    //thread safe (for the socket thread) getter and setter for bools
    void setBool(bool & target, bool newval);

    bool getBool(const bool& val);


};


#endif // !LIBSIM_CONTROL_MODULE_H
