#ifndef CONNECT_LIB_SIM_H
#define CONNECT_LIB_SIM_H

#include <module/module.h>

class ConnectLibSim : public vistle::Module
{
public:
    ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm);
    ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm, const std::string &shmArray);
    int updateParameter(const char* info);
    //adds all available data to the according outputs to execute the pipeline
    bool sendData();
    void SimulationTimeStepChanged();
    void SimulationInitiateCommand(const char* command);
    void DeleteData();

    ~ConnectLibSim() override;
    //set callbacks
    void SetSimulationCommandCallback(void(*sc)(const char*, const char*, void*), void* scdata);
private:
    vistle::StringParameter* p_data_path = nullptr;
    std::vector<vistle::Object::ptr> m_dataObjects;
    std::vector<vistle::Port*> m_portsList;

    //callbacks
    void (*simulationCommandCallback)(const char*, const char*, void*);
    void* simulationCommandCallbackData;

};





#endif // !CONNECT_LIB_SIM_H
