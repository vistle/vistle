#include "Engine.h"

#include <mpi.h>

#include "VisItDataInterfaceRuntime.h"
#include "SimulationMetaData.h"
#include "VisItDataTypes.h"
#include "VertexTypesToVistle.h"
#include "Exeption.h"
#include "TransformArray.h"


#include "MeshMetaData.h"
#include "VariableMetaData.h"
#include "MaterialMetaData.h"
#include "SpeciesMetaData.h"
#include "CurveMetaData.h"
#include "MessageMetaData.h"
#include "CommandMetaData.h"
#include "ExpressionMetaData.h"
#include "DomainList.h"
#include "RectilinearMesh.h"
#include "CurvilinearMesh.h"
#include "UnstructuredMesh.h"
#include "VariableData.h"

#include <boost/mpi.hpp>
#include <control/hub.h>


#include <module/module.h>

#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>
#include <core/unstr.h>

#include <core/message.h>
#include <core/messagequeue.h>
#include <core/tcpmessage.h>

#include <util/sleep.h>
#include <util/directory.h>
#include <util/hostname.h>
#include <util/listenv4v6.h>

#include <ostream>


#ifdef MODULE_THREAD
#include "EngineInterface.cpp"
#endif // MODULE_THREAD


#ifdef  LIBSIM_DEBUG
#define DEBUG_CERR std::cerr << "Engine: " << " [" << m_rank << "/" << m_mpiSize << "] "
#else
#include <insitu/util/print.h>
#define DEBUG_CERR vistle::DoNotPrintInstance
#endif
#define CERR std::cerr << "Engine: " << " [" << m_rank << "/" << m_mpiSize << "] "


using std::string; using std::vector;
using std::endl;
using namespace insitu;
using namespace insitu::message;
namespace asio = boost::asio;
Engine* Engine::instance = nullptr;

Engine* Engine::EngineInstance() {
    if (!instance) {
        instance = new Engine{};
    }
    return instance;

}

void Engine::DisconnectSimulation() {

    instance->m_messageHandler.send(ConnectionClosed{true});
    delete instance;
    instance = nullptr;
}

bool Engine::initialize(int argC, char** argV) {
    m_moduleInfo.id = false;
    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &m_rank);
    MPI_Comm_size(comm, &m_mpiSize);
    CERR<< "__________Engine args__________" << endl;
    for (size_t i = 0; i < argC; i++) {
        CERR << argV[i] << endl;
    }
    CERR << "_______________________________" << endl;
#ifdef MODULE_THREAD
    
    
    if (m_rank == 0) {
        ConnectMySelf();
    }
    // start manager on cluster
    const char* VISTLE_ROOT = getenv("VISTLE_ROOT");
    if (!VISTLE_ROOT) {
        CERR << "VISTLE_ROOT not set to the path of the Vistle build directory." << endl;
        return false;
    }

    m_managerThread = std::thread([argC, argV, VISTLE_ROOT]() {
        std::string cmd{VISTLE_ROOT};
        cmd += "/bin/vistle_manager";
        std::vector<char*> args;
        args.push_back(const_cast<char*>(cmd.c_str()));
        for (int i = 1; i < argC; ++i) {
            args.push_back(argV[i]);
        }
        vistle::VistleManager manager;
        manager.run(args.size(), args.data());
    });

#else
if (argC != 7) {
        CERR << "simulation requires exactly 7 parameters" << endl;
        return false;
    }

    if (atoi(argV[0]) != m_mpiSize) {
        CERR << "mpi size of simulation must match vistle's mpi size" << endl;
        return false;
    }
    m_moduleInfo.shmName = argV[1];
    m_moduleInfo.name = argV[2];
    m_moduleInfo.id = atoi(argV[3]);
    m_moduleInfo.hostname = argV[4];
    m_moduleInfo.port = atoi(argV[5]);
    m_moduleInfo.numCons = argV[6];
    if (m_rank == 0 && argV[4] != vistle::hostname())         {
        CERR << "this " << vistle::hostname() << "trying to connect to " << argV[4] << endl;
        CERR << "Wrong host: must connect to Vistle on the same machine!" << endl;
        return false;
    }

    initializeVistleEnv();
#endif
    

    
    return true;
}


bool Engine::initializeVistleEnv()
{
    vistle::registerTypes();
    vistle::Shm::attach(m_moduleInfo.shmName, m_moduleInfo.id, m_rank);
    vistle::message::DefaultSender::init(m_moduleInfo.id, m_rank);
    // names are swapped relative to communicator
    std::string mqName = vistle::message::MessageQueue::createName(("recvFromSim" + m_moduleInfo.numCons).c_str(), m_moduleInfo.id, m_rank);
    try {
        m_sendMessageQueue = vistle::message::MessageQueue::open(mqName);
    }
    catch (boost::interprocess::interprocess_exception& ex) {
        CERR << "opening send message queue " << mqName << ": " << ex.what() << endl;
        return false;
    }

    if (m_rank == 0) {
        try {
            connectToModule(m_moduleInfo.hostname, m_moduleInfo.port);
        }
        catch (const EngineExeption& ex) {
            CERR << ex.what() << endl;
            return false;
        }
    }
    m_messageHandler.initialize(m_socket, boost::mpi::communicator(comm, boost::mpi::comm_create_kind::comm_duplicate));
    m_messageHandler.send(GoOn());
    try {
        m_shmIDs.initialize(m_moduleInfo.id, m_rank, std::stoi(m_moduleInfo.numCons), SyncShmIDs::Mode::Attach);
    }
    catch (const vistle::exception&) {
        CERR << "failed to initialize SyncShmMessage" << endl;
        return false;
    }

    m_initialized = true;

}

bool Engine::isInitialized() const noexcept {
    return m_initialized;
}

bool Engine::setMpiComm(void* newconn) {
    comm = (MPI_Comm)newconn;

return true;

}
#ifdef MODULE_THREAD
void Engine::ConnectMySelf()     {
    unsigned short port = m_port;

    boost::system::error_code ec;

    while (!vistle::start_listen(port, *m_acceptorv4, *m_acceptorv6, ec)) {
        if (ec == boost::system::errc::address_in_use) {
            ++port;
        } else if (ec) {
            CERR << "failed to listen on port " << port << ": " << ec.message() << endl;
            return;
        }
    }
    m_port = port;
    startAccept(m_acceptorv4);
    startAccept(m_acceptorv6);
    connectToModule("localhost", m_port);

    std::unique_lock<std::mutex> lk(m_asioMutex);
    m_connectedCondition.wait(lk, [this]() {return m_waitingForAccept; });
    m_waitingForAccept = false;

}

bool Engine::startAccept(std::shared_ptr<acceptor> a) {

    if (!a->is_open())
        return false;

    std::shared_ptr<socket> sock(new boost::asio::ip::tcp::socket(m_ioService));
    EngineInterface::setControllSocket(sock);
    boost::system::error_code ec;
    a->async_accept(*sock, [this, a, sock](boost::system::error_code ec) {
        if (ec) {
            m_socket = nullptr;
            CERR << "failed connection attempt" << endl;
            return;
        }
        CERR << "server connected with engine" << (a == m_acceptorv4? "v4" : "v6") <<  endl;
        m_socket = sock;
       
        });
    return true;
}

#endif
bool Engine::sendData() {

    CERR << "sendData was called" << endl;
    return true;
}

void Engine::SimulationTimeStepChanged() {
    if (!m_initialized || !m_moduleInfo.initialized) {
        CERR << "not connected with Vistle. \nStart the ConnectLibSIm module to use simulation data in Vistle!" << endl;
        return;
    }

    static int counter = 0;
    DEBUG_CERR << "SimulationTimeStepChanged counter = " << counter << endl;
    ++counter;
    getMetaData();
    if (m_processedCycles >= m_metaData.currentCycle) {
        DEBUG_CERR << "There is no new timestep but SimulationTimeStepChanged was called" << endl;
        return;
    }
    int numMeshes, numVars;
    v2check(simv2_SimulationMetaData_getNumMeshes, m_metaData.handle, numMeshes);
    v2check(simv2_SimulationMetaData_getNumVariables, m_metaData.handle, numVars);
    if (m_metaData.currentCycle % m_intOptions[message::InSituMessageType::NthTimestep]->val != 0) {
        return;
    }
    DEBUG_CERR << "Timestep " << m_metaData.currentCycle << " has " << numMeshes << " meshes and " << numVars << "variables" << endl;
    if (m_moduleInfo.ready) {//only here vistle::objects are allowed to be made
        sendDataToModule();
        m_processedCycles = m_metaData.currentCycle;
        ++m_timestep;
    }
    else {
        CERR << "ConnectLibSim is not ready to process data" << endl;
    }
}

void Engine::SimulationInitiateCommand(const std::string& command) {
    static int counter = 0;
    DEBUG_CERR << "SimulationInitiateCommand " << command << " counter = " << counter << endl;
    ++counter;
    if (command.substr(0, 12) == "INTERNALSYNC") { //need to respond or LibSim gets stuck. see: SimEngine::SimulationInitiateCommand
        // Send the command back to the engine so it knows we're done syncing. 
        std::string cmd("INTERNALSYNC");
        std::string args(command.substr(13, command.size() - 1));
        simulationCommandCallback(cmd.c_str(), args.c_str(), simulationCommandCallbackData);
        m_messageHandler.send(GoOn{}); //request tcp message from conroller
    }
}

void Engine::DeleteData() {
    DEBUG_CERR << "DeleteData called" << endl;
    m_meshes.clear();
    sendData();
}

bool insitu::Engine::recvAndhandleVistleMessage() {
    static int counter = 0;
    DEBUG_CERR << "handleVistleMessage counter = " << counter << endl;
    ++counter;
    if (m_rank == 0) {
        if (!slaveCommandCallback) {
            CERR << "passCommandToSim failed : slaveCommandCallback not set" << endl;
            return true;;
        }
        slaveCommandCallback(); //let the slaves call visitProcessEngineCommand() -> simv2_process_input() -> Engine::passCommandToSim() and therefore finalizeInit() if not already done
    }
    getMetaData();
    finalizeInit();
    InSituTcp::Message msg = m_messageHandler.recv();
    DEBUG_CERR << "received message of type " << static_cast<int>(msg.type()) << endl;
    switch (msg.type()) {
    case InSituMessageType::Invalid:
        break;
    case InSituMessageType::ShmInit:
        break;
    case InSituMessageType::AddObject:
        break;
    case InSituMessageType::SetPorts:
    {
        SetPorts ap = msg.unpackOrCast<SetPorts>();
        m_moduleInfo.connectedPorts.clear();
        for (auto vec : ap.value)             {
            m_moduleInfo.connectedPorts.insert(vec.begin(), vec.end());
        }
    }
        break;
    case InSituMessageType::SetCommands:
        break;
    case InSituMessageType::Ready:
    {
        Ready em = msg.unpackOrCast<Ready>();
        m_moduleInfo.ready = em.value;
        if (m_moduleInfo.ready) {
            vistle::Shm::the().setObjectID(m_shmIDs.objectID());
            vistle::Shm::the().setArrayID(m_shmIDs.arrayID());
            m_timestep = 0;
        }
        else {
            sendShmIds();
        }
    }
    break;
    case InSituMessageType::ExecuteCommand:
    {
        ExecuteCommand exe = msg.unpackOrCast<ExecuteCommand>();
        if (simulationCommandCallback) {
            simulationCommandCallback(exe.value.c_str(), "", simulationCommandCallbackData);
            DEBUG_CERR << "received simulation command: " << exe.value << endl;
            if (m_registeredGenericCommands.find(exe.value) == m_registeredGenericCommands.end()) {
                DEBUG_CERR << "Engine received unknown command!" << endl;
            }
        } else {

            CERR << "received command, but required callback is not set" << endl;
        }
        m_messageHandler.send(GoOn{});
    }
    break;
    case InSituMessageType::GoOn:
    {
        if (m_metaData.simMode == VISIT_SIMMODE_RUNNING) {
            //m_messageHandler.send(GoOn{});
        }
    }
        break;
    case InSituMessageType::ConnectionClosed:
    {
        CERR << "connection closed" << endl;
        m_shmIDs.close();
        return false;
    }
    break;
#ifdef MODULE_THREAD
    case InSituMessageType::ModuleID:
    {
        auto mid = msg.unpackOrCast<ModuleID>();
        m_moduleID = mid.value;
        std::string mqName = vistle::message::MessageQueue::createName("recvFromSim0" , m_moduleID, m_rank);
        try {
            m_sendMessageQueue = vistle::message::MessageQueue::open(mqName);
        } catch (boost::interprocess::interprocess_exception & ex) {
            CERR << "opening send message queue " << mqName << ": " << ex.what() << endl;
            return false;
        }
        m_messageHandler.initialize(m_socket, boost::mpi::communicator(comm, boost::mpi::comm_create_kind::comm_duplicate));
        m_shmIDs.initialize(m_moduleID, m_rank, 0, message::SyncShmIDs::Mode::Attach);
        m_moduleReady = false;
        
    }
    break;
#endif
    default:
        m_intOptions[msg.type()]->setVal(msg);
        break;
    }
    return true;
}

void Engine::SetSimulationCommandCallback(void(*sc)(const char*, const char*, void*), void* scdata) {
    simulationCommandCallback = sc;
    simulationCommandCallbackData = scdata;
}

void insitu::Engine::setSlaveComandCallback(void(*sc)(void)) {
    DEBUG_CERR << "setSlaveComandCallback" << endl;
    slaveCommandCallback = sc;
}

int insitu::Engine::GetInputSocket() {
    if (m_rank == 0 && m_socket) {
        return m_socket->native_handle();
    }
    else {
        return 0;
    }
}

//...................................................................................................
//----------------private----------------------------------------------------------------------------
//...............get meta date.......................................................................

void insitu::Engine::getMetaData() {
    m_metaData.handle = simv2_invoke_GetMetaData(); //somehow 0 is valid
    v2check(simv2_SimulationMetaData_getData, m_metaData.handle, m_metaData.simMode, m_metaData.currentCycle, m_metaData.currentTime);
}

int Engine::getNumObjects(SimulationDataTyp type) {

    std::function<int(visit_handle, int&)> getNum;
    switch (type) {
    case SimulationDataTyp::mesh:
    {
        getNum = simv2_SimulationMetaData_getNumMeshes;
    }
    break;
    case SimulationDataTyp::variable:
    {
        getNum = simv2_SimulationMetaData_getNumVariables;
    }
    break;
    case SimulationDataTyp::material:
    {
        getNum = simv2_SimulationMetaData_getNumMaterials;
    }
    break;
    case SimulationDataTyp::curve:
    {
        getNum = simv2_SimulationMetaData_getNumCurves;
    }
    break;
    case SimulationDataTyp::expression:
    {
        getNum = simv2_SimulationMetaData_getNumExpressions;
    }
    break;
    case SimulationDataTyp::species:
    {
        getNum = simv2_SimulationMetaData_getNumSpecies;
    }
    break;
    case SimulationDataTyp::genericCommand:
    {
        getNum = simv2_SimulationMetaData_getNumGenericCommands;
    }
    break;
    case SimulationDataTyp::customCommand:
    {
        getNum = simv2_SimulationMetaData_getNumCustomCommands;
    }
    break;
    case SimulationDataTyp::message:
    {
        getNum = simv2_SimulationMetaData_getNumMessages;
    }
    break;
    default:
        throw EngineExeption("getDataNames called with invalid type") << (int)type;

        break;
    }

    int num = 0;
    v2check(getNum, m_metaData.handle, num);
    return num;
}

visit_handle Engine::getNthObject(SimulationDataTyp type, int n) {
    std::function<int(visit_handle, int, visit_handle&)> getObj;
    switch (type) {
    case SimulationDataTyp::mesh:
    {
        getObj = simv2_SimulationMetaData_getMesh;
    }
    break;
    case SimulationDataTyp::variable:
    {
        getObj = simv2_SimulationMetaData_getVariable;
    }
    break;
    case SimulationDataTyp::material:
    {
        getObj = simv2_SimulationMetaData_getMaterial;
    }
    break;
    case SimulationDataTyp::curve:
    {
        getObj = simv2_SimulationMetaData_getCurve;
    }
    break;
    case SimulationDataTyp::expression:
    {
        getObj = simv2_SimulationMetaData_getExpression;
    }
    break;
    case SimulationDataTyp::species:
    {
        getObj = simv2_SimulationMetaData_getSpecies;
    }
    break;
    case SimulationDataTyp::genericCommand:
    {
        getObj = simv2_SimulationMetaData_getGenericCommand;
    }
    break;
    case SimulationDataTyp::customCommand:
    {
        getObj = simv2_SimulationMetaData_getCustomCommand;
    }
    break;
    case SimulationDataTyp::message:
    {
        getObj = simv2_SimulationMetaData_getMessage;
    }
    break;
    default:
        throw EngineExeption("getDataNames called with invalid type");
        break;
    }
    visit_handle obj = m_metaData.handle;
    v2check(getObj, obj, n, obj);
    return obj;
}

std::vector<std::string> insitu::Engine::getDataNames(SimulationDataTyp type) {
    std::function<int(visit_handle, char**)> getName;
    switch (type) {
    case SimulationDataTyp::mesh:
    {
        getName = simv2_MeshMetaData_getName;
    }
    break;
    case SimulationDataTyp::variable:
    {
        getName = simv2_VariableMetaData_getName;
    }
    break;
    case SimulationDataTyp::material:
    {
        getName = simv2_MaterialMetaData_getName;
    }
    break;
    case SimulationDataTyp::curve:
    {
        getName = simv2_CurveMetaData_getName;
    }
    break;
    case SimulationDataTyp::expression:
    {
        getName = simv2_ExpressionMetaData_getName;
    }
    break;
    case SimulationDataTyp::species:
    {
        getName = simv2_SpeciesMetaData_getName;
    }
    break;
    case SimulationDataTyp::genericCommand:
    {
        getName = simv2_CommandMetaData_getName;
    }
    break;
    case SimulationDataTyp::customCommand:
    {
        getName = simv2_CommandMetaData_getName;
    }
    break;
    case SimulationDataTyp::message:
    {
        getName = simv2_MessageMetaData_getName;
    }
    break;
    default:
        throw EngineExeption("getDataNames called with invalid type");
        break;
    }

    int n = getNumObjects(type);
    std::vector<string> names;
    names.reserve(n);
    for (size_t i = 0; i < n; i++) {
        visit_handle obj = getNthObject(type, i);
        char* name;
        v2check(getName, obj, &name);
        names.push_back(name);
    }
    return names;
}

void insitu::Engine::getRegisteredGenericCommands() {
    int numRegisteredCommands = getNumObjects(SimulationDataTyp::genericCommand);
    bool found = false;
    std::vector<std::string> commands;
    for (size_t i = 0; i < numRegisteredCommands; i++) {
        visit_handle commandHandle = getNthObject(SimulationDataTyp::genericCommand, i);
        char* name;
        v2check(simv2_CommandMetaData_getName, commandHandle, &name);
        CERR << "registerd generic command: " << name << endl;
        m_registeredGenericCommands.insert(name);
        commands.push_back(name);
    }
    m_messageHandler.send(SetCommands{ commands });
}

void Engine::addPorts() {
    try {
        SetPorts::value_type ports;
        std::vector<string> names = getDataNames(SimulationDataTyp::mesh);
        names.push_back("mesh");
        ports.push_back(names);

        names = getDataNames(SimulationDataTyp::variable);
        names.push_back("variable");
        ports.push_back(names);
        m_messageHandler.send(SetPorts{ ports });

    } catch (const SimV2Exeption& ex) {
        CERR << "failed to add output ports: " << ex.what() << endl;
    } catch (const EngineExeption & ex) {
        CERR << "failed to add output ports: " << ex.what() << endl;
    }
}
//...................................................................................................
template<typename T, typename...Args>
typename T::ptr Engine::createVistleObject(Args&&... args)     {
    auto obj = typename T::ptr(new T(args...));
    sendShmIds();
    return obj;
}



void insitu::Engine::sendDataToModule() {
    try {
        sendMeshesToModule();
    } catch (const VistleLibSimExeption & exept) {
        CERR << "sendMeshesToModule failed: " << exept.what() << endl;
    }

    try {
        sendVarablesToModule();
    } catch (const VistleLibSimExeption & exept) {
        CERR << "sendVarablesToModule failed: " << exept.what() << endl;
    }
}

void insitu::Engine::sendMeshesToModule()     {
    int numMeshes = getNumObjects(SimulationDataTyp::mesh);

    for (size_t i = 0; i < numMeshes; i++) {
        MeshInfo meshInfo;
        visit_handle meshHandle = getNthObject(SimulationDataTyp::mesh, i);
        char* name;
        v2check(simv2_MeshMetaData_getName, meshHandle, &name);
        auto meshIt = m_meshes.find(name);
        if (m_intOptions[message::InSituMessageType::ConstGrids]->val && meshIt != m_meshes.end()) {
            for (auto grid : meshIt->second.grids) {
                addObject(name, grid);
            }
            return; //the mesh is consistent and already there
        }
        visit_handle domainListHandle = v2check(simv2_invoke_GetDomainList, name);
        int allDoms = 0;
        visit_handle myDoms;
        v2check(simv2_DomainList_getData, domainListHandle, allDoms, myDoms);
        DEBUG_CERR << "allDoms = " << allDoms << " myDoms = " << myDoms << endl;

        int owner, dataType, nComps;
        void* data;
        try {
            v2check(simv2_VariableData_getData, myDoms, owner, dataType, nComps, meshInfo.numDomains, data);
        } catch (const SimV2Exeption&) {
            CERR << "failed to get domain list" << endl;
        }
        if (dataType != VISIT_DATATYPE_INT) {
            CERR << "expected domain list to be ints" << endl;
        }
        meshInfo.domains = static_cast<const int*>(data);
  

        VisIt_MeshType meshType = VisIt_MeshType::VISIT_MESHTYPE_UNKNOWN;
        v2check(simv2_MeshMetaData_getMeshType, meshHandle, (int*)&meshType);


        v2check(simv2_MeshMetaData_getName, meshHandle, &meshInfo.name);
        v2check(simv2_MeshMetaData_getTopologicalDimension, meshHandle, &meshInfo.dim);

        switch (meshType) {
        case VISIT_MESHTYPE_RECTILINEAR:
        {
            CERR << "making rectilinear grid" << endl;
            if (m_intOptions[message::InSituMessageType::CombineGrids]->val) {
                combineRectilinearToUnstructured(meshInfo);
            } else {
                makeRectilinearMesh(meshInfo);
            }
        }
        break;
        case VISIT_MESHTYPE_CURVILINEAR:
        {
            CERR << "making curvilinear grid" << endl;
            if (!m_intOptions[message::InSituMessageType::CombineGrids]->val) {
                makeStructuredMesh(meshInfo);
            }
            else {
                //to do:read domain boundaries if set
                combineStructuredMeshesToUnstructured(meshInfo);
            }

        }
        break;
        case VISIT_MESHTYPE_UNSTRUCTURED:
        {
            CERR << "making unstructured grid" << endl;
            makeUnstructuredMesh(meshInfo);
        }
        break;
        case VISIT_MESHTYPE_POINT:
        {
            CERR << "making point grid" << endl;
        }
        break;
        case VISIT_MESHTYPE_CSG:
        {
            CERR << "making csg grid" << endl;
        }
        break;
        case VISIT_MESHTYPE_AMR:
        {
            CERR << "making AMR grid" << endl;
            makeAmrMesh(meshInfo);
        }
        break;
        default:
            throw EngineExeption("unknown meshtype");
            break;
        }
    }
}

void insitu::Engine::makeRectilinearMesh(MeshInfo meshInfo) {
    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {
        int currDomain = meshInfo.domains[cd];
        visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);
        int check = simv2_RectilinearMesh_check(meshHandle);
        if (check == VISIT_OKAY) {
            visit_handle coordHandles[3]; //handles to variable data
            int ndims;
            v2check(simv2_RectilinearMesh_getCoords, meshHandle, &ndims, &coordHandles[0], &coordHandles[1], &coordHandles[2]);
            std::array<int, 3> owner{}, dataType{}, nComps{}, nTuples{ 1,1,1 };
            std::array<void*, 3> data{};
            for (int i = 0; i < meshInfo.dim; ++i) {
                v2check(simv2_VariableData_getData, coordHandles[i], owner[i], dataType[i], nComps[i], nTuples[i], data[i]);
                if (dataType[i] != dataType[0]) {
                    CERR << "mesh data type must be consistent within a domain" << endl;
                    return;
                }
            }
            //std::reverse(owner.begin(), owner.end());
            //std::reverse(dataType.begin(), dataType.end());
            //std::reverse(nComps.begin(), nComps.end());
            //std::reverse(nTuples.begin(), nTuples.end());
            //std::reverse(data.begin(), data.end());

            vistle::RectilinearGrid::ptr grid = createVistleObject<vistle::RectilinearGrid>(nTuples[0], nTuples[1], nTuples[2]);
            setTimestep(grid);
            grid->setBlock(currDomain);
            meshInfo.handles.push_back(meshHandle);
            meshInfo.grids.push_back(grid);
            for (size_t i = 0; i < 3; ++i) {
                if (data[i]) {
                    transformArray(data[i], grid->coords(i).begin(), nTuples[i], dataType[i]);
                }
                else {
                    grid->coords(i)[0] = 0;
                }
            }

            std::array<int, 3> min, max;
            v2check(simv2_RectilinearMesh_getRealIndices, meshHandle, min.data(), max.data());
            //std::reverse(min.begin(), min.end());
            //std::reverse(max.begin(), max.end());
            for (size_t i = 0; i < 3; i++) {
                assert(min[i] >= 0);
                int numTop = nTuples[i] - 1 - max[i];
                assert(numTop >= 0);
                grid->setNumGhostLayers(i, vistle::StructuredGrid::GhostLayerPosition::Bottom, min[i]);
                grid->setNumGhostLayers(i, vistle::StructuredGrid::GhostLayerPosition::Top, numTop);
            }
            addObject(meshInfo.name, grid);

            DEBUG_CERR << "added rectilinear mesh " << meshInfo.name << " dom = " << currDomain << " xDim = " << nTuples[0] << " yDim = " << nTuples[1] << " zDim = " << nTuples[2] << endl;
            DEBUG_CERR << "min bounds " << min[0] << " " << min[1] << " " << min[2] << " max bouns " << max[0] << " " << max[1] << " " << max[2] << endl;

        }
    }
    DEBUG_CERR << "made rectilinear grids with " << meshInfo.numDomains << " domains" << endl;
    m_meshes[meshInfo.name] = meshInfo;
}

void insitu::Engine::makeUnstructuredMesh(MeshInfo meshInfo) {

    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {
        int currDomain = meshInfo.domains[cd];
        visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);
        int check = simv2_UnstructuredMesh_check(meshHandle);
        if (check == VISIT_OKAY) {
            vistle::UnstructuredGrid::ptr grid = createVistleObject<vistle::UnstructuredGrid>(0, 0, 0);
            std::array<vistle::UnstructuredGrid::array*, 3> gridPoints{ &grid->x(), &grid->y(), &grid->z() };
            //std::array<std::array<int, 10>, 3> gridPoints;
            int ncells;
            visit_handle connListHandle;
            v2check(simv2_UnstructuredMesh_getConnectivity, meshHandle, &ncells, &connListHandle);
            grid->el().reserve(ncells);
            grid->tl().reserve(ncells);
            int owner{}, dataType{}, nComps{}, nTuples{};
            void* data{};
            v2check(simv2_VariableData_getData, connListHandle, owner, dataType, nComps, nTuples, data);
            int idx = 0;
            if (dataType != VISIT_DATATYPE_INT)
            {
                throw EngineExeption("element list is not of expected type (int)");
            }
            //auto elementList = createVistleObject<
            //grid->el().push_back(0);
            size_t elemIndex = 0;
            for (int i = 0; i < ncells; ++i)
            {
                auto type = static_cast<int*>(data)[idx];
                ++idx;
                auto elemType = vertexTypeToVistle(type);
                if (elemType == vistle::UnstructuredGrid::Type::NONE)
                {
                    CERR << "Engine warning: type " << type << " is not converted to a vistle type...returning" << endl;
                    return;
                }

                elemIndex += getNumVertices(elemType);

                if (idx + getNumVertices(elemType) > nTuples)
                {
                    CERR << "element " << i << " with idx = " << idx << " and type " << elemType << "with typesize" << getNumVertices(elemType) << " is outside of given elementList data!" << endl;
                    break;
                }
                grid->el().push_back(elemIndex);
                grid->tl().push_back(elemType);

                for (size_t i = 0; i < getNumVertices(elemType); i++)
                {
                    grid->cl().push_back(static_cast<int*>(data)[idx]);
                    ++idx;
                }
            }

            visit_handle coordHandles[4]; //handles to variable data pos 3 is for interleaved data
            int ndims;
            int coordMode;
            v2check(simv2_UnstructuredMesh_getCoords, meshHandle, &ndims, &coordMode, &coordHandles[0], &coordHandles[1], &coordHandles[2], &coordHandles[3]);

            switch (coordMode) {
            case VISIT_COORD_MODE_INTERLEAVED:
            {
                int owner{}, dataType{}, nComps{}, nTuples{1};
                void* data{};
                v2check(simv2_VariableData_getData, coordHandles[3], owner, dataType, nComps, nTuples, data);
                std::array<vistle::Scalar*, 3> gridCoords;

                for (size_t i = 0; i < meshInfo.dim; ++i)
                {
                    gridPoints[i]->resize(nTuples);
                    gridCoords[i] = gridPoints[i]->data();
                }
                transformInterleavedArray(data, gridCoords, nTuples, dataType, ndims);

            }
            break;
            case VISIT_COORD_MODE_SEPARATE:
            {
                std::array<int, 3> owner{}, dataType{}, nComps{}, nTuples{ 1,1,1 };
                std::array<void*, 3> data{};


                for (int i = 0; i < meshInfo.dim; ++i) {
                    v2check(simv2_VariableData_getData, coordHandles[i], owner[i], dataType[i], nComps[i], nTuples[i], data[i]);
                    gridPoints[i]->resize(nTuples[i]);

                    transformArray(data[i], gridPoints[i]->data(), nTuples[i], dataType[i]);
                }
            }
            break;
            default:
                throw EngineExeption("coord mode must be interleaved(1) or separate(0), it is " + std::to_string(coordMode));
            }


            std::array<int, 3> min, max;
            visit_handle ghostCellHandle;
            v2check(simv2_UnstructuredMesh_getGhostCells, meshHandle, &ghostCellHandle);
            v2check(simv2_UnstructuredMesh_getRealIndices, meshHandle, min.data(), max.data());

            //for (size_t i = 0; i < 3; i++) {
            //    assert(min[i] >= 0);
            //    int numTop = nTuples[i] - 1 - max[i];
            //    assert(numTop >= 0);
            //    grid->setNumGhostLayers(i, vistle::StructuredGrid::GhostLayerPosition::Bottom, min[i]);
            //    grid->setNumGhostLayers(i, vistle::StructuredGrid::GhostLayerPosition::Top, numTop);
            //}
            setTimestep(grid);
            grid->setBlock(currDomain);
            meshInfo.handles.push_back(meshHandle);
            meshInfo.grids.push_back(grid);
            addObject(meshInfo.name, grid);

            DEBUG_CERR << "added unstructured grid with size " << grid->getSize() << ", with " << grid->el().size() << " elements and " << grid->cl().size() << " connections" << endl;
        }
    }
    DEBUG_CERR << "made unstructured grids with " << meshInfo.numDomains << " domains" << endl;
    m_meshes[meshInfo.name] = meshInfo;


    return;
}

void insitu::Engine::makeAmrMesh(MeshInfo meshInfo) {
    if (m_intOptions[message::InSituMessageType::CombineGrids]->val) {
        combineRectilinearToUnstructured(meshInfo);
    } else {
        makeRectilinearMesh(meshInfo);
    }
    


}

void insitu::Engine::makeStructuredMesh(MeshInfo meshInfo) {
    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {
        int currDomain = meshInfo.domains[cd];
        visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);
        int check = simv2_CurvilinearMesh_check(meshHandle);
        if (check == VISIT_OKAY) {
            visit_handle coordHandles[4]; //handles to variable data, 4th entry contains interleaved data depending on coordMode
            int dims[3]{ 1,1,1 }; //the x,y,z dimensions
            int ndims, coordMode;
            //no v2check because last visit_handle can be invalid
            if (!simv2_CurvilinearMesh_getCoords(meshHandle, &ndims, dims, &coordMode, &coordHandles[0], &coordHandles[1], &coordHandles[2], &coordHandles[3]))                 {
                throw EngineExeption("makeStructuredMesh: simv2_CurvilinearMesh_getCoords failed");
            }
            vistle::StructuredGrid::ptr grid = createVistleObject<vistle::StructuredGrid>(dims[0], dims[1], dims[2]);

            std::array<float*, 3> gridCoords{ grid->x().data() ,grid->y().data() ,grid->z().data() };
            int numVals = dims[0] * dims[1] * dims[2];
            int owner{}, dataType{}, nComps{}, nTuples{};
            void* data{};
            switch (coordMode) {
            case VISIT_COORD_MODE_INTERLEAVED:
            {
                v2check(simv2_VariableData_getData, coordHandles[3], owner, dataType, nComps, nTuples, data);
                if (nTuples != numVals * ndims) {
                    throw EngineExeption("makeStructuredMesh: received points in interleaved grid " + std::string(meshInfo.name) + " do not match the grid dimensions");
                }
               
                transformInterleavedArray(data, gridCoords, numVals, dataType, ndims);

            }
            break;
            case VISIT_COORD_MODE_SEPARATE:
            {
            for (int i = 0; i < meshInfo.dim; ++i) {
                v2check(simv2_VariableData_getData, coordHandles[i], owner, dataType, nComps, nTuples, data);
                if (nTuples != numVals) {
                    throw EngineExeption("makeStructuredMesh: received points in separate grid " + std::string(meshInfo.name) + " do not match the grid dimension " + std::to_string(i));
                }
                transformArray(data, gridCoords[i], nTuples, dataType);
            }
            }
            break;
            default:
                throw EngineExeption("coord mode must be interleaved(1) or separate(0), it is " + std::to_string(coordMode));
            }
            if (meshInfo.dim == 2) {
                std::fill(gridCoords[2], gridCoords[2] + numVals, 0);
            }
            setTimestep(grid);
            grid->setBlock(currDomain);
            meshInfo.handles.push_back(meshHandle);
            meshInfo.grids.push_back(grid);
            addObject(meshInfo.name, grid);
        }
    }
    CERR << "made structured mesh with " << meshInfo.numDomains << " domains" << endl;
    m_meshes[meshInfo.name] = meshInfo;
}

void insitu::Engine::combineStructuredMeshesToUnstructured(MeshInfo meshInfo)     {
    using namespace vistle;

    size_t totalNumElements = 0, totalNumVerts = 0;
    size_t numVertices;
    size_t numElements;
    int numCorners = meshInfo.dim == 2 ? 4 : 8;
    vistle::UnstructuredGrid::ptr grid= createVistleObject<vistle::UnstructuredGrid>(0,0,0);
    std::array<float*, 3> gridCoords{ grid->x().end() ,grid->y().end() ,grid->z().end() };
    visit_handle coordHandles[4]; //handles to variable data, 4th entry conteins interleaved data depending on coordMode
    int dims[3]{ 1,1,1 }; //the x,y,z dimensions
    int ndims, coordMode;
    visit_handle meshHandle;

    int owner{}, dataType{}, nComps{}, nTuples{};
    void* data{};


    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {
        int currDomain = meshInfo.domains[cd];
        meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);


        //no v2check because last visit_handle can be invalid
        if (!simv2_CurvilinearMesh_getCoords(meshHandle, &ndims, dims, &coordMode, &coordHandles[0], &coordHandles[1], &coordHandles[2], &coordHandles[3])) {
            throw EngineExeption("makeStructuredMesh: simv2_CurvilinearMesh_getCoords failed");
        }

        numVertices = dims[0] * dims[1] * dims[2];
        numElements = (dims[0] - 1) * (dims[1] - 1) * (dims[2] == 1 ? 1 : (dims[2] - 1));

        // reserve memory for the arrays, /assume we have the same sub-grid size for the rest to reduce the amout of re-allocations
        if (grid->x().size() <totalNumVerts + numVertices) {
            auto newSize = totalNumVerts + numVertices * (meshInfo.numDomains - cd);
            grid->setSize(newSize);
            gridCoords = { grid->x().begin() + totalNumVerts ,grid->y().begin() + totalNumVerts ,grid->z().begin() + totalNumVerts };
        }
        if (grid->cl().size() < (totalNumElements + numElements) * numCorners) {
            grid->cl().resize((totalNumElements + numElements * (meshInfo.numDomains - cd))* numCorners);
        }
        if (m_intOptions[message::InSituMessageType::VTKVariables]->val) {
            //to do: create connectivity list for vtk structured grids
        } else {
            makeStructuredGridConnectivityList(dims, grid->cl().begin() + totalNumElements * numCorners, totalNumVerts);
        }

        switch (coordMode) {
        case VISIT_COORD_MODE_INTERLEAVED:
        {
            v2check(simv2_VariableData_getData, coordHandles[3], owner, dataType, nComps, nTuples, data);
            if (nTuples != numVertices * ndims) {
                throw EngineExeption("makeStructuredMesh: received points in interleaved grid " + std::string(meshInfo.name) + " do not match the grid dimensions");
            }

            transformInterleavedArray(data, gridCoords, numVertices, dataType, ndims);

        }
        break;
        case VISIT_COORD_MODE_SEPARATE:
        {
            for (int i = 0; i < meshInfo.dim; ++i) {
                v2check(simv2_VariableData_getData, coordHandles[i], owner, dataType, nComps, nTuples, data);
                if (nTuples != numVertices) {
                    throw EngineExeption("makeStructuredMesh: received points in separate grid " + std::string(meshInfo.name) + " do not match the grid dimension " + std::to_string(i));
                }
                transformArray(data, gridCoords[i], nTuples, dataType);
            }
        }
        break;
        default:
            throw EngineExeption("coord mode must be interleaved(1) or separate(0), it is " + std::to_string(coordMode));
        }
        gridCoords = { gridCoords[0] + numVertices, gridCoords[1] + numVertices, gridCoords[2] + numVertices, };
        totalNumElements += numElements;
        totalNumVerts += numVertices;

    }
    assert(grid->getSize() >= totalNumVerts);
    grid->setSize(totalNumVerts);
    assert(grid->cl().size() >= totalNumElements * numCorners);
    grid->cl().resize(totalNumElements* numCorners);
    if (meshInfo.dim == 2) {
        std::fill(grid->z().begin(), grid->z().end(), 0);
    }
    grid->tl().resize(totalNumElements);
    std::fill(grid->tl().begin(), grid->tl().end(), meshInfo.dim == 2 ? (const Byte)UnstructuredGrid::QUAD : (const Byte)UnstructuredGrid::HEXAHEDRON);
    grid->el().resize(totalNumElements + 1);
    for (size_t i = 0; i < totalNumElements + 1; i++) {
        grid->el()[i] = numCorners * i;
    }

    setTimestep(grid);
    grid->setBlock(m_rank);
    addObject(meshInfo.name, grid);
    meshInfo.grids.push_back(grid);
    meshInfo.combined = true;
    m_meshes[meshInfo.name] = meshInfo;

    DEBUG_CERR << "added unstructured mesh " << meshInfo.name << " with " << totalNumVerts << " vertices and " << totalNumElements << " elements " << endl;
   }

void insitu::Engine::combineRectilinearToUnstructured(MeshInfo meshInfo) {
    using namespace vistle;

    size_t totalNumElements = 0, totalNumVerts = 0;
    size_t numVertices;
    size_t numElements;
    int numCorners = meshInfo.dim == 2 ? 4 : 8;
    vistle::UnstructuredGrid::ptr grid = createVistleObject<vistle::UnstructuredGrid>(0, 0, 0);
    std::array<float*, 3> gridCoords{ grid->x().end() ,grid->y().end() ,grid->z().end() };
    visit_handle coordHandles[3]; //handles to variable data, 4th entry conteins interleaved data depending on coordMode
    int ndims;
    visit_handle meshHandle;

    int owner{}, dataType[2]{}, nComps[3]{}, nTuples[3]{};
    void* data[3]{};


    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {
        int currDomain = meshInfo.domains[cd];
        meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);


        //no v2check because last visit_handle can be invalid
        if (!simv2_RectilinearMesh_getCoords(meshHandle, &ndims, &coordHandles[0], &coordHandles[1], &coordHandles[2])) {
            throw EngineExeption("makeStructuredMesh: simv2_CurvilinearMesh_getCoords failed");
        }
        for (int i = 0; i < meshInfo.dim; ++i) {
            v2check(simv2_VariableData_getData, coordHandles[i], owner, dataType[i], nComps[i], nTuples[i], data[i]);
            if (dataType[i] != dataType[0]) {
                CERR << "mesh data type must be consisten within a domain" << endl;
                continue;
            }
        }

        if (nTuples[2] == 0) {
            nTuples[2] = 1;
            data[2] = (void*)&zero;
        }
        numVertices = nTuples[0] * nTuples[1] * nTuples[2];
        numElements = (nTuples[0] - 1) * (nTuples[1] - 1) * (nTuples[2] == 1 ? 1 : (nTuples[2] - 1));

        // reserve memory for the arrays, /assume we have the same sub-grid size for the rest to reduce the amout of re-allocations
        if (grid->x().size() < totalNumVerts + numVertices) {
            auto newSize = totalNumVerts + numVertices * (meshInfo.numDomains - cd);
            grid->setSize(newSize);
            gridCoords = { grid->x().begin() + totalNumVerts ,grid->y().begin() + totalNumVerts ,grid->z().begin() + totalNumVerts };
        }
        if (grid->cl().size() < (totalNumElements + numElements) * numCorners) {
            grid->cl().resize((totalNumElements + numElements * (meshInfo.numDomains - cd)) * numCorners);
        }
        makeStructuredGridConnectivityList(nTuples, grid->cl().begin() + totalNumElements * numCorners, totalNumVerts);

        if (m_intOptions[message::InSituMessageType::VTKVariables]->val) {
            expandRectilinearToVTKStructured(data, dataType[0], nTuples, gridCoords);
            makeVTKStructuredGridConnectivityList(nTuples, grid->cl().begin() + totalNumElements * numCorners, totalNumVerts);
        } else {
            expandRectilinearToStructured(data, dataType[0], nTuples, gridCoords);
            makeStructuredGridConnectivityList(nTuples, grid->cl().begin() + totalNumElements * numCorners, totalNumVerts);
        }



        gridCoords = { gridCoords[0] + numVertices, gridCoords[1] + numVertices, gridCoords[2] + numVertices, };
        totalNumElements += numElements;
        totalNumVerts += numVertices;

    }
    assert(grid->getSize() >= totalNumVerts);
    grid->setSize(totalNumVerts);
    assert(grid->cl().size() >= totalNumElements * numCorners);
    grid->cl().resize(totalNumElements * numCorners);
    if (meshInfo.dim == 2) {
        std::fill(grid->z().begin(), grid->z().end(), 0);
    }
    grid->tl().resize(totalNumElements);
    std::fill(grid->tl().begin(), grid->tl().end(), meshInfo.dim == 2 ? (const Byte)UnstructuredGrid::QUAD : (const Byte)UnstructuredGrid::HEXAHEDRON);
    grid->el().resize(totalNumElements + 1);
    for (size_t i = 0; i < totalNumElements + 1; i++) {
        grid->el()[i] = numCorners * i;
    }

    setTimestep(grid);
    grid->setBlock(m_rank);
    addObject(meshInfo.name, grid);
    meshInfo.grids.push_back(grid);
    meshInfo.combined = true;
    m_meshes[meshInfo.name] = meshInfo;

    DEBUG_CERR << "added unstructured mesh " << meshInfo.name << " with " << totalNumVerts << " vertices and " << totalNumElements << " elements " << endl;
}

void insitu::Engine::sendVarablesToModule()     { //todo: combine variables to vectors
    int numVars = getNumObjects(SimulationDataTyp::variable);
    for (size_t i = 0; i < numVars; i++) {
        visit_handle varMetaHandle = getNthObject(SimulationDataTyp::variable, i);
        char* name, *meshName;
        v2check(simv2_VariableMetaData_getName, varMetaHandle, &name);
        if (m_moduleInfo.connectedPorts.find(name) == m_moduleInfo.connectedPorts.end()) { //don't process data for un-used ports
            continue;
        }
        v2check(simv2_VariableMetaData_getMeshName, varMetaHandle, &meshName);
        int centering = -1;
        v2check(simv2_VariableMetaData_getCentering, varMetaHandle, &centering);
        auto meshInfo = m_meshes.find(meshName);
        if (meshInfo == m_meshes.end()) {
            EngineExeption ex{""};
            ex << "sendVarablesToModule: can't find mesh " << meshName << " for variable " << name;
            throw ex;
        }
        vistle::Vec<vistle::Scalar, 1>::ptr variable;
        size_t combinedVecPos = 0;
        if (meshInfo->second.combined) {

            auto unstr = vistle::UnstructuredGrid::as(meshInfo->second.grids[0]);
            if (!unstr) {
                throw EngineExeption{ "combined grids must be UnstructuredGrids" };
            }
            size_t size = centering == VISIT_VARCENTERING_NODE ? unstr->getNumVertices() : unstr->getNumElements();
            auto var = createVistleObject<vistle::Vec<vistle::Scalar, 1>>(size);
            var->setGrid(unstr);
            var->setMapping(centering == VISIT_VARCENTERING_NODE ? vistle::DataBase::Vertex : vistle::DataBase::Element);
            variable = var;
        }



        for (size_t cd = 0; cd < meshInfo->second.numDomains; ++cd) {
            int currDomain = meshInfo->second.domains[cd];
            visit_handle varHandle = v2check(simv2_invoke_GetVariable, currDomain, name);
            int  owner{}, dataType{}, nComps{}, nTuples{};
            void* data = nullptr;
            v2check(simv2_VariableData_getData, varHandle, owner, dataType, nComps, nTuples, data);
            DEBUG_CERR << "added variable " << name << " to mesh " << meshName << " dom = " << currDomain << " Dim = " << nTuples << endl;
            if (meshInfo->second.combined) {
                assert(combinedVecPos + nTuples >= variable->x().size());
                transformArray(data, variable->x().data() + combinedVecPos, nTuples, dataType);
                combinedVecPos += nTuples;
                

            } else {
                if (m_intOptions[message::InSituMessageType::VTKVariables]->val) {

                    auto var = vtkData2Vistle(data, nTuples, dataType, meshInfo->second.grids[cd], centering == VISIT_VARCENTERING_NODE ? vistle::DataBase::Vertex : vistle::DataBase::Element);
                    sendShmIds(); //since vtkData2Vistle creates objects
                    auto vec = std::dynamic_pointer_cast<vistle::Vec<vistle::Scalar, 1>>(var);

                    if (!vec) {
                        CERR << "sendVarablesToModule failed to convert variable " << name << "... trying next" << endl;
                        continue;
                    }
                    variable = vec;
                } else {
                    variable = createVistleObject<vistle::Vec<vistle::Scalar, 1>>(nTuples);
                    transformArray(data, variable->x().data(), nTuples, dataType);
                    variable->setGrid(meshInfo->second.grids[cd]);
                    variable->setMapping(centering == VISIT_VARCENTERING_NODE ? vistle::DataBase::Vertex : vistle::DataBase::Element);
                }
                setTimestep(variable);
                variable->setBlock(currDomain);
                variable->addAttribute("_species", name);
                addObject(name, variable);
            }
        }
        if (meshInfo->second.combined) {
            setTimestep(variable);
            variable->setBlock(m_rank);
            variable->addAttribute("_species", name);
            addObject(name, variable);
        }
        DEBUG_CERR << "sent variable " << name << " with " << meshInfo->second.numDomains << " domains" << endl;
    }
}

void insitu::Engine::sendTestData() {

    int rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::array<int, 3> dims{ 5, 10, 1 };
    vistle::RectilinearGrid::ptr grid = createVistleObject<vistle::RectilinearGrid>(dims[0], dims[1], dims[2]);
    setTimestep(grid);
    grid->setBlock(rank);
    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < dims[i]; j++) {
            switch (i) {
            case 0:
                grid->coords(i)[j] = j;
                break;
            case 1:
                grid->coords(i)[j] = m_metaData.currentCycle * dims[i] + j;
                break;
            case 2:
                grid->coords(i)[j] = rank * dims[i] + j;
                break;
            default:
                break;
            }
        }
    }
    CERR << "sending test data for cycle " << m_metaData.currentCycle << endl;
    addObject("AMR_mesh", grid);
}

void insitu::Engine::connectToModule(const std::string& hostname, int port) {

    boost::system::error_code ec;
    asio::ip::tcp::resolver resolver(m_ioService);
    asio::ip::tcp::resolver::query query(hostname, boost::lexical_cast<std::string>(port));
    m_socket.reset(new boost::asio::ip::tcp::socket(m_ioService));
    asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
    if (ec) {
        throw EngineExeption("initializeEngineSocket failed to resolve connect socket");
    }
#ifndef MODULE_THREAD
    asio::connect(*m_socket, endpoint_iterator, ec);
#else
    asio::async_connect(*m_socket, endpoint_iterator, [this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator iterator) {
        if (ec) {
            m_socket = nullptr;
            CERR << "client failed to connect" << endl;
            return;
        }
        CERR << "client connected with engine" << endl;
        {
            std::lock_guard<std::mutex> g(m_asioMutex);
            m_waitingForAccept = true;
        }
        m_connectedCondition.notify_one();
    });
#endif
    if (ec) {
        throw EngineExeption("initializeEngineSocket failed to connect socket");
    }




}

void insitu::Engine::finalizeInit()     {
    if (!m_moduleInfo.initialized) {
        try {
            getMetaData();
            addPorts();
            getRegisteredGenericCommands();

        } catch (const VistleLibSimExeption& ex) {
            CERR << "finalizeInit failed: " << ex.what() << endl;
            return;
        }
        m_moduleInfo.initialized = true;
    }
}

void insitu::Engine::addObject(const std::string& name, vistle::Object::ptr obj) {
    if (m_sendMessageQueue) {
        DEBUG_CERR << "addObject " << name << " of type " << obj->typeName() << endl;
        vistle::message::AddObject msg(name, obj);
        vistle::message::Buffer buf(msg);

#ifdef MODULE_THREAD
        buf.setSenderId(m_moduleID);
        buf.setRank(m_rank);
#endif
        m_sendMessageQueue->send(buf);
    }


}

void insitu::Engine::makeStructuredGridConnectivityList(const int* dims, vistle::Index * connectivityList, vistle::Index startOfGridIndex)     {
    // construct connectivity list (all hexahedra)
    using namespace vistle;
    Index numVert[3];
    Index numElements[3];
    for (size_t i = 0; i < 3; ++i) {
        numVert[i] = static_cast<Index>(dims[i]);
        numElements[i] = numVert[i] - 1;
    }
    if (dims[2] > 1) {
        // 3-dim
        Index el = 0;
        for (Index ix = 0; ix < numElements[0]; ++ix) {
            for (Index iy = 0; iy < numElements[1]; ++iy) {
                for (Index iz = 0; iz < numElements[2]; ++iz) {
                    const Index baseInsertionIndex = el * 8;
                    connectivityList[baseInsertionIndex + 0] = startOfGridIndex + UniformGrid::vertexIndex(ix, iy, iz, numVert);                   // 0       7 -------- 6
                    connectivityList[baseInsertionIndex + 1] = startOfGridIndex + UniformGrid::vertexIndex(ix + 1, iy, iz, numVert);               // 1      /|         /|
                    connectivityList[baseInsertionIndex + 2] = startOfGridIndex + UniformGrid::vertexIndex(ix + 1, iy + 1, iz, numVert);           // 2     / |        / |
                    connectivityList[baseInsertionIndex + 3] = startOfGridIndex + UniformGrid::vertexIndex(ix, iy + 1, iz, numVert);               // 3    4 -------- 5  |
                    connectivityList[baseInsertionIndex + 4] = startOfGridIndex + UniformGrid::vertexIndex(ix, iy, iz + 1, numVert);               // 4    |  3-------|--2
                    connectivityList[baseInsertionIndex + 5] = startOfGridIndex + UniformGrid::vertexIndex(ix + 1, iy, iz + 1, numVert);           // 5    | /        | /
                    connectivityList[baseInsertionIndex + 6] = startOfGridIndex + UniformGrid::vertexIndex(ix + 1, iy + 1, iz + 1, numVert);       // 6    |/         |/
                    connectivityList[baseInsertionIndex + 7] = startOfGridIndex + UniformGrid::vertexIndex(ix, iy + 1, iz + 1, numVert);           // 7    0----------1

                    ++el;
                }
            }
        }
    } else if (dims[1] > 1) {
        // 2-dim
        Index el = 0;
        for (Index ix = 0; ix < numElements[0]; ++ix) {
            for (Index iy = 0; iy < numElements[1]; ++iy) {
                const Index baseInsertionIndex = el * 4;
                connectivityList[baseInsertionIndex + 0] = startOfGridIndex + UniformGrid::vertexIndex(ix, iy, 0, numVert);
                connectivityList[baseInsertionIndex + 1] = startOfGridIndex + UniformGrid::vertexIndex(ix + 1, iy, 0, numVert);
                connectivityList[baseInsertionIndex + 2] = startOfGridIndex + UniformGrid::vertexIndex(ix + 1, iy + 1, 0, numVert);
                connectivityList[baseInsertionIndex + 3] = startOfGridIndex + UniformGrid::vertexIndex(ix, iy + 1, 0, numVert);
                ++el;
            }
        }
    } 
}

void insitu::Engine::makeVTKStructuredGridConnectivityList(const int* dims, vistle::Index* connectivityList, vistle::Index startOfGridIndex, vistle::Index(*vertexIndex)(vistle::Index, vistle::Index, vistle::Index, vistle::Index[3])) {
    // construct connectivity list (all hexahedra)
    using namespace vistle;
    
    if (!vertexIndex) {
        vertexIndex = [](vistle::Index x, vistle::Index y, vistle::Index z, vistle::Index dims[3]) {
            return VTKVertexIndex(x, y, z, dims);
        };
    }
    Index numVert[3];
    Index numElements[3];
    for (size_t i = 0; i < 3; ++i) {
        numVert[i] = static_cast<Index>(dims[i]);
        numElements[i] = numVert[i] - 1;
    }
    if (dims[2] > 1) {
        // 3-dim
        Index el = 0;
        for (Index ix = 0; ix < numElements[0]; ++ix) {
            for (Index iy = 0; iy < numElements[1]; ++iy) {
                for (Index iz = 0; iz < numElements[2]; ++iz) {
                    const Index baseInsertionIndex = el * 8;
                    connectivityList[baseInsertionIndex + 0] = startOfGridIndex + vertexIndex(ix, iy, iz, numVert);                   // 0       7 -------- 6
                    connectivityList[baseInsertionIndex + 1] = startOfGridIndex + vertexIndex(ix + 1, iy, iz, numVert);               // 1      /|         /|
                    connectivityList[baseInsertionIndex + 2] = startOfGridIndex + vertexIndex(ix + 1, iy + 1, iz, numVert);           // 2     / |        / |
                    connectivityList[baseInsertionIndex + 3] = startOfGridIndex + vertexIndex(ix, iy + 1, iz, numVert);               // 3    4 -------- 5  |
                    connectivityList[baseInsertionIndex + 4] = startOfGridIndex + vertexIndex(ix, iy, iz + 1, numVert);               // 4    |  3-------|--2
                    connectivityList[baseInsertionIndex + 5] = startOfGridIndex + vertexIndex(ix + 1, iy, iz + 1, numVert);           // 5    | /        | /
                    connectivityList[baseInsertionIndex + 6] = startOfGridIndex + vertexIndex(ix + 1, iy + 1, iz + 1, numVert);       // 6    |/         |/
                    connectivityList[baseInsertionIndex + 7] = startOfGridIndex + vertexIndex(ix, iy + 1, iz + 1, numVert);           // 7    0----------1

                    ++el;
                }
            }
        }
    } else if (dims[1] > 1) {
        // 2-dim
        Index el = 0;
        for (Index ix = 0; ix < numElements[0]; ++ix) {
            for (Index iy = 0; iy < numElements[1]; ++iy) {
                const Index baseInsertionIndex = el * 4;
                connectivityList[baseInsertionIndex + 0] = startOfGridIndex + vertexIndex(ix, iy, 0, numVert);
                connectivityList[baseInsertionIndex + 1] = startOfGridIndex + vertexIndex(ix + 1, iy, 0, numVert);
                connectivityList[baseInsertionIndex + 2] = startOfGridIndex + vertexIndex(ix + 1, iy + 1, 0, numVert);
                connectivityList[baseInsertionIndex + 3] = startOfGridIndex + vertexIndex(ix, iy + 1, 0, numVert);
                ++el;
            }
        }
    }
}


void Engine::setTimestep(vistle::Object::ptr data)     {
    int step;
    if (m_intOptions[message::InSituMessageType::ConstGrids]->val) {
        step = -1;
    }
    else {
        step = m_timestep;
    }
    if (m_intOptions[message::InSituMessageType::KeepTimesteps]->val) {
        data->setTimestep(step);
    } else {
        data->setIteration(step);
    }

}

void Engine::setTimestep(vistle::Vec<vistle::Scalar, 1>::ptr vec)     {
    if (m_intOptions[message::InSituMessageType::KeepTimesteps]->val) {
        vec->setTimestep(m_timestep);
    } else {
        vec->setIteration(m_timestep);
    }
}

void Engine::sendShmIds()     {
    m_shmIDs.set(vistle::Shm::the().objectID(), vistle::Shm::the().arrayID() );
}

Engine::Engine()
#ifdef MODULE_THREAD
#if BOOST_VERSION >= 106600
: m_workGuard(boost::asio::make_work_guard(m_ioService))
#else
 :m_workGuard(new boost::asio::io_service::work(m_ioService))
#endif
, m_ioThread([this]() {
    m_ioService.run();
    DEBUG_CERR << "io thread terminated" << endl;
    })
{

    m_acceptorv4.reset(new boost::asio::ip::tcp::acceptor(m_ioService));
    m_acceptorv6.reset(new boost::asio::ip::tcp::acceptor(m_ioService));
#else
    {
#endif

    m_intOptions[message::InSituMessageType::ConstGrids] = std::unique_ptr< IntOption<message::ConstGrids>>(new IntOption<message::ConstGrids>{false});
    m_intOptions[message::InSituMessageType::VTKVariables] = std::unique_ptr< IntOption<message::VTKVariables>>(new IntOption<message::VTKVariables>{false});
    m_intOptions[message::InSituMessageType::NthTimestep] = std::unique_ptr< IntOption<message::NthTimestep>>(new IntOption<message::NthTimestep>{1});
    m_intOptions[message::InSituMessageType::CombineGrids] = std::unique_ptr< IntOption<message::CombineGrids>>{ new IntOption<message::CombineGrids>{ false , [this]() {m_meshes.clear(); }} };
    m_intOptions[message::InSituMessageType::KeepTimesteps] = std::unique_ptr< IntOption<message::KeepTimesteps>>(new IntOption<message::KeepTimesteps>{ false });
}

Engine::~Engine() {
    m_meshes.clear();
#ifndef MODULE_THREAD
    if (vistle::Shm::isAttached()) {
        vistle::Shm::the().detach();
    }
#endif
    delete m_sendMessageQueue;
    Engine::instance = nullptr;
}







