#ifndef VISIT_VISTLE_ENGINE_H
#define VISIT_VISTLE_ENGINE_H

#include <mpi.h>
#include "ModuleInterface.h"


class V_VISITXPORT Engine {
public:
    static Engine* createEngine();
    static void DisconnectSimulation();
    bool initialize(int argC, char** argV);
    bool setMpiComm(void *newConn);

    //adds all available data to the according outputs to execute the pipeline
    bool sendData();
    void SimulationTimeStepChanged();
    void SimulationInitiateCommand(const char* command);
    void DeleteData();
    //set callbacks
    void SetSimulationCommandCallback(void(*sc)(const char*, const char*, void*), void* scdata);


private:
    static Engine* instance;
    MPI_Comm comm = MPI_COMM_WORLD;
    LibSimModuleInterface* m_module = nullptr;


    //callbacks
    void (*simulationCommandCallback)(const char*, const char*, void*);
    void* simulationCommandCallbackData;


    Engine() = default;
    ~Engine();
};





#endif // !VISIT_VISTLE_ENGINE_H
