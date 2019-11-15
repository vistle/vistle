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

    
    return true;
}

bool Engine::setMpiComm(void* newconn) {


    comm = (MPI_Comm)newconn;

return true;

}


bool Engine::sendData() {
    if (!m_module) {
        return false;
    }
    if (!m_module->sendData()) {
        return false;
    }
    return true;
}

void Engine::SimulationTimeStepChanged() {
}

void Engine::SimulationInitiateCommand(const char* command) {
}



void Engine::DeleteData() {
    if (m_module) {
        m_module->DeleteData();
    }
}



void Engine::SetSimulationCommandCallback(void(*sc)(const char*, const char*, void*), void* scdata) {
    simulationCommandCallback = sc;
    simulationCommandCallbackData = scdata;
}



Engine::~Engine() {
    delete instance;
}


