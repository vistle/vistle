#ifndef VISIT_VISTLE_ENGINE_H
#define VISIT_VISTLE_ENGINE_H

#include <mpi.h>
#include "ConnectLibSim.h"


class Engine {
public:
    static Engine* createEngine();
    static void DisconnectSimulation();
    bool initialize(int argC, char** argV);
    bool setMpiComm(void *newConn);
    ConnectLibSim* module();
private:
    static Engine* instance;
    MPI_Comm comm = MPI_COMM_WORLD;
    ConnectLibSim* m_module = nullptr;
    Engine() = default;
    ~Engine();
};





#endif // !VISIT_VISTLE_ENGINE_H
