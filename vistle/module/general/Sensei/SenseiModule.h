#ifndef SENSEI_CONTROL_MODULE_H
#define SENSEI_CONTROL_MODULE_H
#include <insitu/module/inSituReader.h>


#include <core/message.h>
#include <core/messagequeue.h>

#include <insitu/message/InSituMessage.h>
#include <insitu/message/SyncShmIDs.h>
#include <insitu/message/ShmMessage.h>

class SenseiModule : public insitu::InSituReader
{
public:

    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;

    typedef std::lock_guard<std::mutex> Guard;

    SenseiModule(const std::string& name, int moduleID, mpi::communicator comm);
    ~SenseiModule();

    

private:
    insitu::message::InSituShmMessage m_messageHandler;


    vistle::StringParameter* m_filePath = nullptr;
    vistle::IntParameter* m_timeout = nullptr;

    bool m_simInitSent = false; //to prevent caling attemptLibSImConnection twice
    bool m_connectedToSim = false; //wether the socket connection to the engine is running
    bool m_firstConnectionAttempt = true;
    std::map<std::string, vistle::Port*> m_outputPorts; //output ports for the data the simulation offers
    std::set<vistle::Parameter*> m_commandParameter; //buttons to trigger simulation commands
    //...................................................................................
    //used to manage the int and bool options this module always offers
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
        IntParam(vistle::IntParameter* param, const insitu::message::InSituShmMessage& sender)
            :IntParamBase(param) 
        , m_sender(sender){
        }
        const insitu::message::InSituShmMessage& m_sender;
        virtual void send() override {
            m_sender.send(T{ static_cast<typename T::value_type>(param()->getValue()) });
        }
    };
    std::map<insitu::message::InSituMessageType, std::unique_ptr<IntParamBase>> m_intOptions;
    //..........................................................................
    //module functions

    virtual bool beginExecute() override;
    virtual bool endExecute() override;
    virtual bool changeParameter(const vistle::Parameter* param);
    //..........................................................................



    
    void recvAndhandleMessage();

    void handleMessage(insitu::message::Message& msg);

    void connectToSim();

    void disconnectSim();

};


#endif // !SENSEI_CONTROL_MODULE_H
