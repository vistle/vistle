#include "Engine.h"
#include <boost/mpi.hpp>

#include <manager/manager.h>

#include <module/module.h>

#include "VisItDataInterfaceRuntime.h"
#include "SimulationMetaData.h"
#include "VisItDataTypes.h"

#include "MeshMetaData.h"
#include "VariableMetaData.h"
#include "MaterialMetaData.h"
#include "SpeciesMetaData.h"
#include "CurveMetaData.h"
#include "MessageMetaData.h"
#include "CommandMetaData.h"
#include "ExpressionMetaData.h"

#include "RectilinearMesh.h"
#include "VariableData.h"

#include <boost/mpi.hpp>
#include <control/hub.h>
#include <util/directory.h>
#include <manager/manager.h>
#include <module/module.h>

#include <core/rectilineargrid.h>
#include <core/message.h>
#include <core/tcpmessage.h>






using std::string; using std::vector;
using std::cerr; using std::endl;
using namespace in_situ;
namespace asio = boost::asio;
Engine* Engine::instance = nullptr;

Engine* Engine::createEngine() {
    if (!instance) {
        instance = new Engine;
    }
    return instance;
}

void in_situ::Engine::setModule(vistle::Module* module) {
    m_module = module;
    addPorts();
}

void in_situ::Engine::setDoReadMutex(std::mutex* m) {
    m_doReadMutex = m;
}

bool Engine::getNumObjects(SimulationDataTyp type, int& num) {

    std::function<bool(visit_handle, int&)> getNum;
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
        cerr << "getDataNames called with invalid type" << (int)type << endl;
        return false;
        break;
    }

    visit_handle metaData = VISIT_INVALID_HANDLE;
    metaData = simv2_invoke_GetMetaData();
    if (metaData == VISIT_INVALID_HANDLE) {
        cerr << "Engine Can't get meta data from simulation" << endl;
        return false;
    }
    num = 0;
    if (getNum(metaData, num) == VISIT_ERROR) {
        cerr << "Engine Can't get num from simulation metadata" << endl;
        simv2_FreeObject(metaData);
        return false;
    }
    return true;
}

bool Engine::getNthObject(SimulationDataTyp type, int n, visit_handle& obj) {
    std::function<bool(visit_handle, int, visit_handle&)> getObj;
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
        cerr << "getDataNames called with invalid type" << endl;
        return false;
        break;
    }

    if (getObj(simv2_invoke_GetMetaData(), n, obj) == VISIT_ERROR) {
        cerr << "Engine Can't get obj" << n << "from simulation metadata" << endl;
        return false;
    }
    return true;
}

bool Engine::getDataNames(SimulationDataTyp type, std::vector<std::string>& names) {
    std::function<bool(visit_handle, char**)> getName;
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
        cerr << "getDataNames called with invalid type" << endl;
        return false;
        break;
    }

    int n = 0;
    if (!getNumObjects(type, n)) {
        printToConsole("getDataNames failed to get getNumObjects");
        return false;
    }
    names.clear();
    names.reserve(n);
    for (size_t i = 0; i < n; i++) {
        visit_handle obj;
        if (!getNthObject(type, i, obj)) {
            printToConsole("getDataNames failed to get getNthObject");
            return false;
        }
        char* name;
        if (getName(obj, &name) == VISIT_ERROR) {
            cerr << "getDataNames can't get mesh name for mesh" << i << "from simulation metadata" << endl;
            return false;
        }
        names.push_back(name);
    }
    return true;


}

void Engine::DisconnectSimulation() {

    delete instance;
}

bool Engine::initialize(int argC, char** argV) {
    // start manager on cluster
    printToConsole("__________Engine args__________");
    for (size_t i = 0; i < argC; i++) {
        printToConsole(argV[i]);
    }
    printToConsole("_______________________________");

    const char *VISTLE_ROOT = getenv("VISTLE_ROOT");
    if (!VISTLE_ROOT) {
        printToConsole("VISTLE_ROOT not set");
        return false;
    }

    managerThread = std::thread([argC, argV, VISTLE_ROOT]() {
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
    return true;

}

bool Engine::isInitialized() const noexcept {
    return m_initialized;
}

bool Engine::setMpiComm(void* newconn) {
    comm = (MPI_Comm)newconn;

return true;

}

bool Engine::sendData() {


    return true;
}

void Engine::SimulationTimeStepChanged() {
    
    if (timestepChangedCb && m_doReadMutex) {
        std::lock_guard<std::mutex> g(*m_doReadMutex);
        
        if (timestepChangedCb())             {
            sendDataToModule();
        }
    }
    else {
        printToConsole("not connected with Vistle. \nStart the ConnectLibSIm module to use simulation data in Vistle!");
    }
}

bool in_situ::Engine::getMetaData(Metadata& md) {
    visit_handle h = simv2_invoke_GetMetaData();
    if (h == VISIT_INVALID_HANDLE) {
        return false;
    }
    if (simv2_SimulationMetaData_getData(h, md.simMode, md.currentCycle,
        md.currentTime) == VISIT_ERROR) {
        cerr << "Engine Can't get data from simulation metadata" << endl;
        simv2_FreeObject(h);
        return false;
    }
    
    cerr << "simMode = " << md.simMode << " currentCycle = " << md.currentCycle << " currentTime = " << md.currentTime << endl;
    return true;
}

void Engine::SimulationInitiateCommand(const char* command) {
}

void Engine::DeleteData() {
    m_meshes.clear();
    sendData();
}

void Engine::SetSimulationCommandCallback(void(*sc)(const char*, const char*, void*), void* scdata) {
    simulationCommandCallback = sc;
    simulationCommandCallbackData = scdata;
}

void in_situ::Engine::SetTimestepChangedCb(std::function<bool(void)> cb) {
    timestepChangedCb = cb;
}

void in_situ::Engine::SetDisconnectCb(std::function<void(void)> cb) {
    disconnectCb = cb;
}

bool Engine::addPorts() {
    if (!m_module) {
        return false;
    }
    std::vector<string> names;
    if (!getDataNames(SimulationDataTyp::mesh, names)) {
        cerr << "ConnectLibSim failed to add mesh ports" << endl;
        return false;
    }
    if (names.size() == 0) {
        cerr << "ConnectLibSim did not find any mesh" << endl;
    }
    for (const auto name : names) {
        auto port = m_module->createOutputPort(name, "mesh");
        m_portsList[name] = port;
    }
    if (!getDataNames(SimulationDataTyp::variable, names)) {
        cerr << "ConnectLibSim failed to add variable ports" << endl;
        return false;
    }
    if (names.size() == 0) {
        cerr << "ConnectLibSim did not find any variables" << endl;
    }
    for (const auto name : names) {
        auto port = m_module->createOutputPort(name, "variable");
        m_portsList[name] = port;
    }
    return true;
}

bool in_situ::Engine::makeCurvilinearMesh(visit_handle h) {
    char* name;
    int dim;
    if (simv2_MeshMetaData_getName(h, &name)) {
        return false;
    }
    if (simv2_MeshMetaData_getTopologicalDimension(h, &dim)) {
        return false;
    }




}

bool in_situ::Engine::makeUntructuredMesh(visit_handle h) {
    char* name;
    int dim;
    if (simv2_MeshMetaData_getName(h, &name)) {
        return false;
    }
    if (simv2_MeshMetaData_getTopologicalDimension(h, &dim)) {
        return false;
    }
    
    return false;
}

bool in_situ::Engine::makeAmrMesh(visit_handle meshMetaHandle) {
    char* name;
    int dim;
    if (!simv2_MeshMetaData_getName(meshMetaHandle, &name)) {
        return false;
    }
    if (!simv2_MeshMetaData_getTopologicalDimension(meshMetaHandle, &dim)) {
        return false;
    }
    int numDomains = 0;
    if (!simv2_MeshMetaData_getNumDomains(meshMetaHandle, &numDomains)) {
        return false;
    }
    MeshInfo mi;
    mi.numDomains = numDomains;
    m_meshes[name] = mi;
    for (size_t i = 0; i < numDomains; i++) {
        visit_handle meshHandle = simv2_invoke_GetMesh(i, name);
        int check = simv2_RectilinearMesh_check(meshHandle);
        printToConsole("invoking get mesh for domain " + std::to_string(i) + " with name " + name + " handle = " + std::to_string(meshHandle) + "check= " + std::to_string(check));

        if (check == VISIT_OKAY) {
            printToConsole("handle ok");
            visit_handle coordHandles[3]; //handles to variable data
            size_t dimensions[3]{ 0,0,0 };
            int ndims;
            if (!simv2_RectilinearMesh_getCoords(meshHandle, &ndims, &coordHandles[0], &coordHandles[1], &coordHandles[2])) {
                return false;
            }
            int owner[3]{}, dataType[3]{}, nComps[3]{}, nTuples[3]{};
            void* data[3]{};
            for (int i = 0; i < dim; ++i) {
                if (simv2_VariableData_getData(coordHandles[i], owner[i], dataType[i],
                    nComps[i], nTuples[i], data[i]) == VISIT_ERROR) {
                    return false;
                }
                cerr << "coord " << i <<  "owner = " << owner[i] << "dataType = " << dataType[i] << " ncomps = " << nComps[i] << "nTuples = " << nTuples[i] << endl;
                if (dataType[i] != VISIT_DATATYPE_FLOAT) {
                    printToConsole("mesh coords must be floats");
                    return false;
                }
            }
            vistle::RectilinearGrid::ptr grid = vistle::RectilinearGrid::ptr(new vistle::RectilinearGrid(nTuples[0], nTuples[1], nTuples[2]));
            m_meshes[name].handles.push_back(meshHandle);
            m_meshes[name].grids.push_back(grid);
            for (size_t i = 0; i < dim; i++) {
                memcpy(grid->coords(i).begin(), data[i], nTuples[i] * sizeof(float));
            }
            m_module->addObject(name, grid);
        }
        return true;
    }



    //vistle::RectilinearGrid::ptr vistleGrid(new vistle::RectilinearGrid());
}

bool in_situ::Engine::sendMeshesToModule()     {
    int numMeshes = 0;
    if (!getNumObjects(SimulationDataTyp::mesh, numMeshes)) {
        return false;
    }
    for (size_t i = 0; i < numMeshes; i++) {
        visit_handle meshHandle = VISIT_INVALID_HANDLE;
        if (!getNthObject(SimulationDataTyp::mesh, i, meshHandle)) {
            return false;
        }
        VisIt_MeshType meshType = VisIt_MeshType::VISIT_MESHTYPE_UNKNOWN;
        simv2_MeshMetaData_getMeshType(meshHandle, (int*)&meshType);
        switch (meshType) {
        case VISIT_MESHTYPE_RECTILINEAR:
        {
            cerr << "making rectilinear grid" << endl;
        }
        break;
        case VISIT_MESHTYPE_CURVILINEAR:
        {
            cerr << "making curvilinear grid" << endl;
        }
        break;
        case VISIT_MESHTYPE_UNSTRUCTURED:
        {
            cerr << "making unstructured grid" << endl;
        }
        break;
        case VISIT_MESHTYPE_POINT:
        {
            cerr << "making point grid" << endl;
        }
        break;
        case VISIT_MESHTYPE_CSG:
        {
            cerr << "making csg grid" << endl;
        }
        break;
        case VISIT_MESHTYPE_AMR:
        {
            cerr << "making amr grid" << endl;
            if (!makeAmrMesh(meshHandle))
                return false;
        }
        break;
        default:
            return false;
            break;
        }
    }
    return true;
}

bool in_situ::Engine::sendVarablesToModule()     {
    int numVars = 0;
    if (!getNumObjects(SimulationDataTyp::variable, numVars)) {
        return false;
    }
    for (size_t i = 0; i < numVars; i++) {
        visit_handle varMetaHandle = VISIT_INVALID_HANDLE;
        if (!getNthObject(SimulationDataTyp::variable, i, varMetaHandle)) {
            return false;
        }
        char* name, *meshName;
        if (!simv2_VariableMetaData_getName(varMetaHandle, &name)) {
            return false;
        }
        if (!simv2_VariableMetaData_getMeshName(varMetaHandle, &meshName)) {
            return false;
        }
        auto meshInfo = m_meshes.find(meshName);
        if (meshInfo == m_meshes.end()) {
            printToConsole(std::string("can't find mesh ") + meshName + " for variable " + name);
            return false;
        }


        for (size_t j = 0; j < meshInfo->second.numDomains; j++) {
            visit_handle varHandle = simv2_invoke_GetVariable(j, name);
            int  owner{}, dataType{}, nComps{}, nTuples{};
            void* data = nullptr;
            if (!simv2_VariableData_getData(varHandle, owner, dataType, nComps, nTuples, data)) {
                return false;
            }
            cerr << "variable " << name << " domain " << j << " owner = " << owner << " dataType = " << dataType << " ncomps = " << nComps << " nTuples = " << nTuples << endl;
            switch (dataType) {
            case VISIT_DATATYPE_CHAR:
            {
                sendVariableToModule(name, meshInfo->second.grids[j], j, (char*)data, nTuples);
                //vistle::Vec<char, 1>::ptr variable(new vistle::Vec<char, 1>(nTuples));
                //memcpy(variable->x().data(), data, nTuples * sizeof(char));
                //variable->setGrid(meshInfo->second.grids[j]);
                ////variable->setTimestep(timestep);
                //variable->setBlock(j);
                //variable->addAttribute("_species", name);
                //m_module->addObject(name, variable);
            }
            break;
            case VISIT_DATATYPE_INT:
            case VISIT_DATATYPE_FLOAT:
            case VISIT_DATATYPE_DOUBLE:
            case VISIT_DATATYPE_LONG:
            case VISIT_DATATYPE_STRING:
            default:
                break;
            }


        }




        
    }
}

bool in_situ::Engine::sendDataToModule() {
    if (!sendMeshesToModule()) {
        return false;
    }
    if (!sendVarablesToModule()) {
        return false;
    }
    return true;

}

Engine::Engine()
{ }

Engine::~Engine() {
    if (disconnectCb) {
        disconnectCb();
    }
   
}

void in_situ::Engine::printToConsole(const std::string& msg) const {
    int rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        cerr << "LibSim::Engine: " << msg << endl;
    }
}






