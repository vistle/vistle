#ifndef CONNECT_LIB_SIM_H
#define CONNECT_LIB_SIM_H

#include <module/module.h>
#include "ModuleInterface.h"

class ConnectLibSim : public vistle::Module, public LibSimModuleInterface
{
public:
    ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm);
    ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm, const std::string &shmArray);
    int updateParameter(const char* info);

    ~ConnectLibSim() override;

    // Inherited via LibSimModuleInterface
    virtual void DeleteData() override;

    virtual bool sendData() override;

    virtual void SimulationTimeStepChanged() override;

    virtual void SimulationInitiateCommand(const char* command) override;

    virtual void SetSimulationCommandCallback(void(*sc)(const char*, const char*, void*), void* scdata) override;
private:

    std::vector<vistle::Port*> m_portsList;
    std::vector<vistle::Object::ptr> m_dataObjects;





};





#endif // !CONNECT_LIB_SIM_H
