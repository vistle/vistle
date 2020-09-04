#include "Engine.h"

#include <mpi.h>

#include "VertexTypesToVistle.h"
#include "Exeption.h"
#include "VisitDataTypesToVistle.h"

#include "RectilinearMesh.h"
#include "UnstructuredMesh.h"
#include "StructuredMesh.h"

#include <vistle/insitu/core/transformArray.h>

#include <vistle/insitu/libsim/libsimInterface/VisItDataInterfaceRuntime.h>
#include <vistle/insitu/libsim/libsimInterface/SimulationMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>
#include <vistle/insitu/libsim/libsimInterface/MeshMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/VariableMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/MaterialMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/SpeciesMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/CurveMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/MessageMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/CommandMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/ExpressionMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/DomainList.h>
#include <vistle/insitu/libsim/libsimInterface/VariableData.h>

#include <boost/mpi.hpp>
#include <vistle/control/hub.h>


#include <vistle/module/module.h>


#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>
#include <vistle/core/tcpmessage.h>

#include <vistle/util/sleep.h>
#include <vistle/util/directory.h>
#include <vistle/util/hostname.h>
#include <vistle/util/listenv4v6.h>

#include <ostream>


#ifdef MODULE_THREAD
#include "EngineInterface.cpp"
#endif // MODULE_THREAD


#ifdef  LIBSIM_DEBUG
#define DEBUG_CERR std::cerr << "Engine: " << " [" << m_rank << "/" << m_mpiSize << "] "
#else
#include <vistle/insitu/util/print.h>
#define DEBUG_CERR vistle::DoNotPrintInstance
#endif
#define CERR std::cerr << "Engine: " << " [" << m_rank << "/" << m_mpiSize << "] "


using std::string; using std::vector;
using std::endl;
using namespace vistle::insitu;
using namespace vistle::insitu::message;
using namespace vistle::insitu::libsim;
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
    m_messageHandler.initialize(m_socket, boost::mpi::communicator(comm, boost::mpi::comm_create_kind::comm_duplicate)); //comm is not set by the simulation code at this point. Since most simulations never change comm, this is a easy cheat impl
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
    m_initialized = true;
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

    return initializeVistleEnv();
#endif
    
   
    return true;
}

bool Engine::initializeVistleEnv()
{
    vistle::registerTypes();
#ifdef SHMPERRANK
    //remove the"_r" + std::to_string(rank) because that gets added by attach again
    char c_ = ' ', cr = ' ';
    while (m_moduleInfo.shmName.size() > 0 &&c_ != '_' && cr != 'r')
    {
        cr = c_;
        c_ = m_moduleInfo.shmName[m_moduleInfo.shmName.size() - 1];
        m_moduleInfo.shmName.pop_back();
    }
    if (m_moduleInfo.shmName.size() == 0)
    {
        CERR << "failed to create shmName: expected it to end with _r + std::to_string(rank)" << endl;
        return false;
    }
#endif // SHMPERRANK

    
    try
    {
        CERR << "attaching to shm: name = " << m_moduleInfo.shmName << " id = " << m_moduleInfo.id << " rank = " << m_rank << endl;
        vistle::Shm::attach(m_moduleInfo.shmName, m_moduleInfo.id, m_rank);
    }
    catch (const vistle::shm_exception& ex)
    {
        CERR << ex.what() << endl;
        return false;
    }
    try {
        m_sendMessageQueue.reset(new AddObjectMsq(m_moduleInfo, m_rank));
    }
    catch (InsituExeption& ex) {
        CERR << "failed to create AddObjectMsq numCons =  " <<  m_moduleInfo.numCons  << " module id = " << m_moduleInfo.id <<  " : " << ex.what() << endl;
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
    return true;
}

bool Engine::isInitialized() const noexcept {
    return m_initialized;
}

bool Engine::setMpiComm(void* newconn) {
    comm = *static_cast<MPI_Comm*>(newconn);

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

}

bool Engine::startAccept(std::shared_ptr<acceptor> a) {

    if (!a->is_open())
        return false;

    std::shared_ptr<socket> sock(new boost::asio::ip::tcp::socket(m_ioService));
    boost::system::error_code ec;
    a->async_accept(*sock, [this, a, sock](boost::system::error_code ec) {
        if (ec) {
            if (m_waitingForAccept)
            {
                EngineInterface::setControllSocket(nullptr);
                CERR << "failed connection attempt" << endl;
            }
            return;
        }
        CERR << "server connected with engine" << (a == m_acceptorv4? "v4" : "v6") <<  endl;
        EngineInterface::setControllSocket(sock);
        m_waitingForAccept = false;
        a == m_acceptorv4 ? m_acceptorv6->close() : m_acceptorv4->close();
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
    if (m_processedCycles > 0 && m_processedCycles >= m_metaData.currentCycle) {
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

bool Engine::recvAndhandleVistleMessage() {
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
    auto msg = m_messageHandler.recv();
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
        m_moduleInfo.connectedPorts.insert(ap.value[0].begin(), ap.value[0].end());
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
            m_messageHandler.send(Ready{ false }); //confirm that we are done creating vistle objects
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
        m_moduleInfo.id = mid.value;
        m_moduleInfo.ready = false;
        m_sendMessageQueue.reset(new AddObjectMsq(m_moduleInfo, m_rank));
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

void Engine::setSlaveComandCallback(void(*sc)(void)) {
    DEBUG_CERR << "setSlaveComandCallback" << endl;
    slaveCommandCallback = sc;
}

int Engine::GetInputSocket() {
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

void Engine::getMetaData() {
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

std::vector<std::string> Engine::getDataNames(SimulationDataTyp type) {
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

void Engine::getRegisteredGenericCommands() {
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
        std::vector<string> meshNames = getDataNames(SimulationDataTyp::mesh);
        meshNames.push_back("mesh");
        ports.push_back(meshNames);

        auto varNames = getDataNames(SimulationDataTyp::variable);
        varNames.push_back("variable");
        ports.push_back(varNames);

        std::vector<string> debugNames;
        int i = 0;
        for (auto mesh : meshNames)
        {
            debugNames.push_back(mesh + "_mpi_ranks");
        }

        debugNames.push_back("debug");
        ports.push_back(debugNames);

        m_messageHandler.send(SetPorts{ ports });

    } catch (const SimV2Exeption& ex) {
        CERR << "failed to add output ports: " << ex.what() << endl;
    } catch (const EngineExeption & ex) {
        CERR << "failed to add output ports: " << ex.what() << endl;
    }
}
//...................................................................................................
void Engine::sendDataToModule() {
    try {
        sendMeshesToModule();
    } catch (const InsituExeption & exept) {
        CERR << "sendMeshesToModule failed: " << exept.what() << endl;
    }

    try {
        sendVarablesToModule();
    } catch (const InsituExeption& exept) {
        CERR << "sendVarablesToModule failed: " << exept.what() << endl;
    }
}

void Engine::sendMeshesToModule()     {
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
                makeRectilinearMeshes(meshInfo);
            }
        }
        break;
        case VISIT_MESHTYPE_CURVILINEAR:
        {
            CERR << "making curvilinear grid" << endl;
            if (!m_intOptions[message::InSituMessageType::CombineGrids]->val) {
                makeStructuredMeshes(meshInfo);
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
            makeUnstructuredMeshes(meshInfo);
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

void Engine::makeRectilinearMeshes(MeshInfo &meshInfo) {
    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {

        makeRectilinearMesh(meshInfo.domains[cd], meshInfo);
    }
    m_meshes[meshInfo.name] = meshInfo;
    DEBUG_CERR << "made " << meshInfo.numDomains << " rectilinear grids" << endl;
}

void vistle::insitu::libsim::Engine::makeRectilinearMesh(int currDomain, MeshInfo& meshInfo)
{
    visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);

    auto mesh = RectilinearMesh::get(meshHandle, m_shmIDs);
    if (!mesh)
    {
        CERR << "makeUnstructuredMeshes failed to get mesh " << meshInfo.name << " domain " << currDomain << endl;
        return;
    }
    finalizeGrid(mesh, currDomain, meshInfo, meshHandle);
    DEBUG_CERR << "added rectilinear mesh " << meshInfo.name << " dom = " << currDomain << endl;
    CERR << "makeRectilinearMesh failed to create mesh " << meshInfo.name << " block " << currDomain << endl;


}

void Engine::makeUnstructuredMeshes(MeshInfo &meshInfo) {

    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {
        makeUnstructuredMesh(meshInfo.domains[cd], meshInfo);
    }
    m_meshes[meshInfo.name] = meshInfo;

    DEBUG_CERR << "made " << meshInfo.numDomains << " unstructured grids" << endl;
}

void Engine::makeUnstructuredMesh(int currDomain, MeshInfo& meshInfo)
{
    visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);
    auto mesh = UnstructuredMesh::get(meshHandle, m_shmIDs);
    if (!mesh)
    {
        CERR << "makeUnstructuredMeshes faile to get mesh " << meshInfo.name << " domain " << currDomain << endl;
        return;
    }
    finalizeGrid(mesh, currDomain, meshInfo, meshHandle);
}

void Engine::finalizeGrid(vistle::Object::ptr grid, int block, MeshInfo& meshInfo, visit_handle meshHandle)
{
    setTimestep(grid);
    grid->setBlock(block);
    meshInfo.handles.push_back(meshHandle);
    meshInfo.grids.push_back(grid);
    addObject(meshInfo.name, grid);
}

void Engine::makeAmrMesh(MeshInfo &meshInfo) {
    if (m_intOptions[message::InSituMessageType::CombineGrids]->val) {
        combineRectilinearToUnstructured(meshInfo);
    } else {
        makeRectilinearMeshes(meshInfo);
    }
}

void Engine::makeStructuredMeshes(MeshInfo &meshInfo) {
    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {
        makeStructuredMesh(meshInfo.domains[cd], meshInfo);
    }
    CERR << "made structured mesh " << meshInfo.name << " with " << meshInfo.numDomains << " domains" << endl;
    m_meshes[meshInfo.name] = meshInfo;
}

void Engine::makeStructuredMesh(int currDomain, MeshInfo& meshInfo)
{
    visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);
    auto mesh = StructuredMesh::get(meshHandle, m_shmIDs);
    if (!mesh)
    {
        CERR << "makeUnstructuredMeshes failed to get mesh " << meshInfo.name << " domain " << currDomain << endl;
        return;
    }
    finalizeGrid(mesh, currDomain, meshInfo, meshHandle);
}

void Engine::combineStructuredMeshesToUnstructured(MeshInfo &meshInfo)     {
    
    auto mesh = StructuredMesh::getCombinedUnstructured(meshInfo, m_shmIDs, m_intOptions[message::InSituMessageType::VTKVariables]->val);
    StructuredMesh::Test( m_shmIDs, 5);
    meshInfo.combined = true;
    finalizeGrid(mesh, m_rank, meshInfo, -1);
    m_meshes[meshInfo.name] = meshInfo;

   }

void Engine::combineRectilinearToUnstructured(MeshInfo &meshInfo) {
    auto mesh = RectilinearMesh::getCombinedUnstructured(meshInfo, m_shmIDs, m_intOptions[message::InSituMessageType::VTKVariables]->val);
    meshInfo.combined = true;
    m_meshes[meshInfo.name] = meshInfo;
    finalizeGrid(mesh, m_rank, meshInfo, -1);

}

void Engine::sendVarablesToModule()     { //todo: combine variables to vectors
    int numVars = getNumObjects(SimulationDataTyp::variable);
    for (size_t i = 0; i < numVars; i++) {
        visit_handle varMetaHandle = getNthObject(SimulationDataTyp::variable, i);
        char* name, *meshName;
        v2check(simv2_VariableMetaData_getName, varMetaHandle, &name);
        if (std::find(m_moduleInfo.connectedPorts.begin(), m_moduleInfo.connectedPorts.end(), name) == m_moduleInfo.connectedPorts.end()) { //don't process data for un-used ports
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
            auto var = m_shmIDs.createVistleObject<vistle::Vec<vistle::Scalar, 1>>(size);
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
                transformArray(data, variable->x().data() + combinedVecPos, nTuples, dataTypeToVistle(dataType));
                combinedVecPos += nTuples;
                

            } else {
                if (m_intOptions[message::InSituMessageType::VTKVariables]->val) {

                    auto var = vtkData2Vistle(data, nTuples, dataTypeToVistle(dataType), meshInfo->second.grids[cd], centering == VISIT_VARCENTERING_NODE ? vistle::DataBase::Vertex : vistle::DataBase::Element);
                    sendShmIds(); //since vtkData2Vistle creates objects
                    auto vec = std::dynamic_pointer_cast<vistle::Vec<vistle::Scalar, 1>>(var);

                    if (!vec) {
                        CERR << "sendVarablesToModule failed to convert variable " << name << "... trying next" << endl;
                        continue;
                    }
                    variable = vec;
                } else {
                    variable = m_shmIDs.createVistleObject<vistle::Vec<vistle::Scalar, 1>>(nTuples);
                    transformArray(data, variable->x().data(), nTuples, dataTypeToVistle(dataType));
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
    for (auto mesh : m_meshes)
    {
        string name = mesh.first + "_mpi_ranks";
        if (std::find(m_moduleInfo.connectedPorts.begin(), m_moduleInfo.connectedPorts.end(), name) != m_moduleInfo.connectedPorts.end())
        {
            for (auto grid : mesh.second.grids)
            {
                auto str = vistle::StructuredGridBase::as(grid);
                auto unstr = vistle::UnstructuredGrid::as(grid);
                vistle::Index mumElements;
                if (str)
                {
                    mumElements = str->getNumElements();
                }
                else
                {
                    mumElements = unstr->getNumElements();
                }
                vistle::Vec<vistle::Scalar, 1>::ptr v = m_shmIDs.createVistleObject<vistle::Vec<vistle::Scalar, 1>>(mumElements);
                std::fill(v->x().begin(), v->x().end(), m_rank);
                v->setMapping(vistle::DataBase::Mapping::Element);
                v->setGrid(grid);
                v->addAttribute("_species", name);
                setTimestep(v);
                addObject(name, v);
            }
        }
    }


}

void Engine::sendTestData() {

    int rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::array<int, 3> dims{ 5, 10, 1 };
    vistle::RectilinearGrid::ptr grid = m_shmIDs.createVistleObject<vistle::RectilinearGrid>(dims[0], dims[1], dims[2]);
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

void Engine::connectToModule(const std::string& hostname, int port) {

    boost::system::error_code ec;
    asio::ip::tcp::resolver resolver(m_ioService);
    asio::ip::tcp::resolver::query query(hostname, boost::lexical_cast<std::string>(port));
    m_socket.reset(new boost::asio::ip::tcp::socket(m_ioService));
    asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
    if (ec) {
        throw EngineExeption("initializeEngineSocket failed to resolve connect socket");
    }

    asio::connect(*m_socket, endpoint_iterator, ec);

    if (ec) {
        throw EngineExeption("initializeEngineSocket failed to connect socket");
    }




}

void Engine::finalizeInit()     {
    if (!m_moduleInfo.initialized) {
        try {
            getMetaData();
            addPorts();
            getRegisteredGenericCommands();

        } catch (const InsituExeption& ex) {
            CERR << "finalizeInit failed: " << ex.what() << endl;
            return;
        }
        m_moduleInfo.initialized = true;
    }
}

void Engine::addObject(const std::string& port, vistle::Object::ptr obj) {
    if (m_sendMessageQueue) {
        m_sendMessageQueue->addObject(port, obj);
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
    Engine::instance = nullptr;
}

