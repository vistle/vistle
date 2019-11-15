#include "ConnectLibSim.h"

#include <vector>
#include <memory>

//MODULE_MAIN(ConnectLibSim)

ConnectLibSim::ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm)
    :vistle::Module("Connect to a simulation that implements the LibSim in-situ interface", name, moduleID, comm) {
    
}

ConnectLibSim::ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm, const std::string & shmname)
    : vistle::Module("Connect to a simulation that implements the LibSim in-situ interface", name, moduleID, comm) {

    vistle::registerTypes(); 
    int rank=-1, size=-1; 

    try { 

    MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
    MPI_Comm_size(MPI_COMM_WORLD, &size); 

    vistle::Module::setup(shmname, moduleID, rank); 
    eventLoop();
    MPI_Barrier(MPI_COMM_WORLD); 
    } 
    catch(vistle::exception &e) { 
    std::cerr << "[" << rank << "/" << size << "]: fatal exception: " << e.what() << std::endl; 
    std::cerr << "  info: " << e.info() << std::endl; 
    std::cerr << e.where() << std::endl; 
    exit(1); 
    } 
    catch(std::exception &e) { 
    std::cerr << "[" << rank << "/" << size << "]: fatal exception: " << e.what() << std::endl; 
    exit(1); 
    } 
}

int ConnectLibSim::updateParameter(const char* info) {
    constexpr int maxNameSize = 100;
    char name[maxNameSize];
    sscanf(info, "%s:", &name);
    info += strlen(name) + 2; //2 to remove the ":"
    auto param = findParameter(name);
    if (!param) {
        return false;
    }
    int enabled;
    switch (param->type()) {
    case vistle::Parameter::Type::Float:
    {
        vistle::Float val;
        sscanf(info, "%lf:%d", &val, &enabled);
        setParameter(name, val);

    }
        break;
    case vistle::Parameter::Type::Integer:
    {
        int val;
        sscanf(info, "%d:%d", &val, &enabled);
        setParameter(name, (vistle::Integer)val);
    }
    break;
    case vistle::Parameter::Type::String:
    {
        std::string val;
        val.resize(maxNameSize);
        sscanf(info, "%s:%d", val.data(), &enabled);
        setParameter(name, val);
    }
    break;
    case vistle::Parameter::Type::Vector:
    {
        vistle::ParamVector val;
        sscanf(info, "%lf,%lf,%fl:%d", &val[0], &val[1], &val[2], &enabled);
        setParameter(name, val);
    }
    break;
    default:
        return false;
        break;
    }
    return true;
}

bool ConnectLibSim::sendData() {
    
    for (size_t i = 0; i < m_dataObjects.size(); ++i) {
        if (!addObject(m_portsList[i], m_dataObjects[i])) {
            std::cerr << "ConnectLibSim: failed to addObject(" << m_portsList[i]->getName() << ", " << m_dataObjects[i]->typeName() << std::endl;
            return false;
        }
    }
    return true;
}

void ConnectLibSim::SimulationTimeStepChanged() {
}

void ConnectLibSim::SimulationInitiateCommand(const char* command) {
}



void ConnectLibSim::DeleteData() {
    for (size_t i = 0; i < m_dataObjects.size(); ++i) {
        m_dataObjects[i].reset(vistle::Object::createEmpty());
    }
    sendData();
}

ConnectLibSim::~ConnectLibSim() {
}

void ConnectLibSim::SetSimulationCommandCallback(void(*sc)(const char*, const char*, void*), void* scdata) {
    simulationCommandCallback = sc;
    simulationCommandCallbackData = scdata;
}

MODULE_MAIN(ConnectLibSim)


