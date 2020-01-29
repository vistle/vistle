#ifndef VISIT_VISTLE_ENGINE_H
#define VISIT_VISTLE_ENGINE_H

#include <mpi.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

#include "MetaData.h"
#include "VisItExports.h"
#include "EngineMessage.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <thread>

#ifdef MODULE_THREAD
#include <manager/manager.h>
#endif // MODULE_THREAD


#include "ConnectLibSim.h"
#include <core/vec.h>
#include <module/module.h>

class ConnectLibSim;
namespace in_situ {
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

    static Engine* createEngine();
    static void DisconnectSimulation();
    bool initialize(int argC, char** argV);
    bool isInitialized() const noexcept;
    bool setMpiComm(void* newConn);


    //********************************
    //***functions called by module***
    //********************************
    int getNumObjects(SimulationDataTyp type);
    visit_handle getNthObject(SimulationDataTyp type, int n);
    std::vector<std::string> getDataNames(SimulationDataTyp type);

    //********************************
    //****functions called by sim****
    //********************************
    //adds all available data to the according outputs to execute the pipeline
    bool sendData();
    //called from simulation when a timestep changed
    void SimulationTimeStepChanged();
    void SimulationInitiateCommand(const std::string& command);
    void DeleteData();
    //called by the sim when m_socket receives data. Reads this data until 
    void passCommandToSim();

    //set callbacks (called from sim)
    void SetSimulationCommandCallback(void(*sc)(const char*, const char*, void*), void* scdata);
    void setSlaveComandCallback(void(*sc)(void));
    int GetInputSocket();


private:
    static Engine* instance;
    bool m_initialized = false; //Engine is initialize

    //mpi info

    int m_rank = -1, m_mpiSize = 0;
    MPI_Comm comm = MPI_COMM_WORLD;

    std::mutex* m_doReadMutex = nullptr;
    //thread to run the vistle manager in
    std::thread managerThread;

//Port info to comunicate with the vistle module
    const unsigned short m_basePort = 31100;
    unsigned short m_port = 0;
    boost::asio::io_service m_ioService;
    std::shared_ptr<acceptor> m_acceptorv4, m_acceptorv6;
    std::shared_ptr<socket> m_socket;
//info from the simulation
    Metadata m_metaData;
    std::set<std::string> registeredGenericCommands;
    struct MeshInfo {
        char* name = nullptr;
        int dim = 0; //2D or 3D
        int numDomains = 0;
        const int* domains = nullptr;
        std::vector<int> handles;
        std::vector< vistle::obj_ptr> grids;
    };
    std::map<std::string, MeshInfo> m_meshes;

    //module info
    bool m_moduleInitialized = false; //Module is initialized(sent port and command info)
    std::string m_shmName, m_moduleName;
    int m_moduleID = 0;
    bool m_moduleReady = false;
    int m_nthTimestep = 1; //how often data should be processed
    bool m_constGrids = false; //if the grids have to be updated for every timestep
    //callbacks from simulation
    void (*simulationCommandCallback)(const char*, const char*, void*) = nullptr;
    void* simulationCommandCallbackData = nullptr;
    void (*slaveCommandCallback)(void);

    //retrieves metaData from simulation 
    void getMetaData();
    //get the commands that the simulation implements and send them to the module
    void getRegisteredGenericCommands();
    //get the data types that the simulation implements and send them to the module to create output ports
    void addPorts();

    bool makeRectilinearMesh(MeshInfo meshInfo);
    bool makeUntructuredMesh(MeshInfo meshInfo);
    bool makeAmrMesh(MeshInfo meshInfo);
    bool makeStructuredMesh(MeshInfo meshInfo);
    void sendMeshesToModule();
    
    void sendVarablesToModule();
    //create all data objects and send them to vistle
    void sendDataToModule();
    void sendTestData();
    //if not already done, initializes the things that require the simulation callbacks
    void finalizeInit();

    template<typename T>
    void sendVariableToModule(const std::string& name, vistle::obj_const_ptr mesh, int domain, const T* data, int size) {
        typename vistle::Vec<T, 1>::ptr variable(new typename vistle::Vec<T, 1>(size));
        memcpy(variable->x().data(), data, size * sizeof(T));
        variable->setGrid(mesh);
        //variable->setTimestep(timestep);
        variable->setBlock(domain);
        variable->setMapping(vistle::DataBase::Element);
        variable->addAttribute("_species", name);
        //m_module->addObject(name, variable);
    }
    void initializeEngineSocket(const std::string &hostname, int port);

    void handleEngineMessage(EngineMessage::Type type);

    void handleEngineMessage(EngineMessage& msg);


    Engine();
    ~Engine();
    
};





}

#endif // !VISIT_VISTLE_ENGINE_H
