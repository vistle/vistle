#ifndef VISIT_VISTLE_ENGINE_H
#define VISIT_VISTLE_ENGINE_H

#include <mpi.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

#include "MetaData.h"
#include "export.h"
#include <insitu/message/InSituMessage.h>

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <thread>

#ifdef MODULE_THREAD
#include <manager/manager.h>
#endif // MODULE_THREAD


#include <core/vec.h>

namespace vistle {
namespace message {
class  MessageQueue;
}
}
namespace insitu {
enum class SimulationDataTyp {
     mesh
    ,variable
    ,material
    ,curve
    ,expression
    ,species
    ,genericCommand
    ,customCommand
    ,message

};

class V_VISITXPORT Engine {
public:
    
    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;

    static Engine* EngineInstance();
    static void DisconnectSimulation();
    bool initialize(int argC, char** argV);
    bool isInitialized() const noexcept;
    bool setMpiComm(void* newConn);


    //********************************
    //***functions called by module***
    //********************************


    //********************************
    //****functions called by sim****
    //********************************
    //oea nothing, should add all available data to the according outputs to execute the pipeline
    bool sendData();
    //called from simulation when a timestep changed
    void SimulationTimeStepChanged();
    //called by the static LibSim library for syncing. So far only handles "INTERNALSYNC" commands.
    //When this function gets called, the simulationCommandCallback has been replaced with and internal callback for the duration of the call.
    void SimulationInitiateCommand(const std::string& command);
    //not sure what to do here. 
    void DeleteData();
    //called by the sim on rank 0 when m_socket receives data and on other ranks when we call slaveCommandCallback.
    //To distribude commands on all ranks, rank 0 calls slaveCommandCallback, reads from socked and broadcasts
    //while the other ranks only do the broadcast.
    //then handels the received Vistle message
    bool recvAndhandleVistleMessage();

    //set callbacks (from sim)
    void SetSimulationCommandCallback(void(*sc)(const char*, const char*, void*), void* scdata);
    //set callbacks (called from sim and from the LibSim static library whyle syncing)
    void setSlaveComandCallback(void(*sc)(void));
    //return the file descripter of m_socket so that LibSim can wait for messages on that socket
    int GetInputSocket();


private:
    static Engine* instance;
    bool m_initialized = false; //Engine is initialized
    vistle::message::MessageQueue* m_sendMessageQueue = nullptr; //Queue to send addObject messages to LibSImController module
    //mpi info
    int m_rank = -1, m_mpiSize = 0;
    MPI_Comm comm = MPI_COMM_WORLD;
#ifdef MODULE_THREAD
    //thread to run the vistle manager in
    std::thread m_managerThread;
#endif
//Port info to comunicate with the vistle module
    unsigned short m_port = 0;
    boost::asio::io_service m_ioService;
    std::shared_ptr<acceptor> m_acceptorv4, m_acceptorv6;
    std::shared_ptr<socket> m_socket;
//info from the simulation
    size_t m_processedCycles = 0; //the last cycle that was processed
    Metadata m_metaData; //the meta data of the currenc cycle
    std::set<std::string> m_registeredGenericCommands; //the commands that are availiable for the sim
    struct MeshInfo {
        bool combined = false; //if the mesh is made of multiple smaler meshes
        char* name = nullptr;
        int dim = 0; //2D or 3D
        int numDomains = 0;
        const int* domains = nullptr;
        std::vector<int> handles;
        std::vector< vistle::obj_ptr> grids;
    };
    std::map<std::string, MeshInfo> m_meshes; //used to find the coresponding mesh for the variables

    //module info
    bool m_moduleInitialized = false; //Module is initialized(sent port and command info)
    std::string m_shmName, m_moduleName;
    int m_moduleID = 0;
    bool m_moduleReady = false; //wether the module is executing or not
    std::set<std::string> m_connectedPorts;
    struct IntOptionBase
    {
        virtual void setVal(insitu::message::InSituTcpMessage& msg) {};
        int val = 0;
    };
    template<typename T>
    struct IntOption : public IntOptionBase  {
        IntOption(int initialVal) {
            val = initialVal;
        }
        virtual void setVal(insitu::message::InSituTcpMessage& msg) override
        {
            
            auto m = msg.unpackOrCast<T>();
            val = static_cast<typename T::value_type>(m.value);
        }
    };
    std::map<insitu::message::InSituMessageType, std::unique_ptr<IntOptionBase>> m_intOptions; // options that can be set in the module
    //callbacks from simulation
    void (*simulationCommandCallback)(const char*, const char*, void*) = nullptr;
    void* simulationCommandCallbackData = nullptr;
    void (*slaveCommandCallback)(void) = nullptr;
    //...................................................................
    //functions to retrieve meta data from sim
    //...................................................................
    //get the meta handle and info about run mode and cycle and store them in m_metaData
    void getMetaData();
    //find out how many objects of type the simulation has
    int getNumObjects(SimulationDataTyp type);
    //get the handle to the nth object of type
    visit_handle getNthObject(SimulationDataTyp type, int n);
    //get a vector of object names
    std::vector<std::string> getDataNames(SimulationDataTyp type);
    //get the commands that the simulation implements and send them to the module
    void getRegisteredGenericCommands();
    //get the data types that the simulation implements and send them to the module to create output ports
    void addPorts();
    //...................................................................
        
    void sendDataToModule();//create all data objects and send them to vistle
    
    void sendMeshesToModule();
    bool makeRectilinearMesh(MeshInfo meshInfo);
    bool makeUntructuredMesh(MeshInfo meshInfo);
    bool makeAmrMesh(MeshInfo meshInfo);
    bool makeStructuredMesh(MeshInfo meshInfo);
    //combine the structured meshes of one domain to a singe unstructured mesh. Points of adjacent faces will be doubled.
    void combineStructuredMeshesToUnstructured(MeshInfo meshInfo);

    
    void sendVarablesToModule();

    void sendTestData(); //testing only

    void connectToModule(const std::string &hostname, int port);
   
    void finalizeInit();  //if not already done, initializes the things that require the simulation 

    void addObject(const std::string& name, vistle::Object::ptr obj); //send addObject message to module, from where it gets passed to Vistle

    void makeStructuredGridConnectivityList(const int* dims, vistle::Index* elementList, vistle::Index startOfGridIndex);

    Engine();

    ~Engine();

    
};





}

#endif // !VISIT_VISTLE_ENGINE_H
