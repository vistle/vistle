#include "Engine.h"

#include <boost/mpi.hpp>

#include <string>

Engine* Engine::instance = nullptr;

Engine* Engine::createEngine() {
    if (!instance) {
        instance = new Engine;
    }
    return instance;
}

void Engine::DisconnectSimulation() {
    delete instance;
}

bool Engine::initialize(int argC, char** argV) {
    if (argC != 3) {
        return false;
    }
    std::string name(argV[0]);
    int moduleID = atoi(argV[1]);
    std::string shmName(argV[2]);

    boost::mpi::communicator c(comm, boost::mpi::comm_create_kind::comm_duplicate);

    if (!m_module) {
        m_module = new ConnectLibSim(name, moduleID, c, shmName);
    }
    
    return true;
}

bool Engine::setMpiComm(void* newconn) {


    comm = (MPI_Comm)newconn;

return true;

}

ConnectLibSim* Engine::module() {
    return m_module;
}

Engine::~Engine() {
    delete instance;
}


