#ifndef MODULEINTERFACE_H
#define MODULEINTERFACE_H

#include "VisItExports.h"

class V_VISITXPORT LibSimModuleInterface {
public:


    virtual void DeleteData() = 0;
    virtual bool sendData() = 0;
    virtual void SimulationTimeStepChanged() = 0;
    virtual void SimulationInitiateCommand(const char* command) = 0;
    virtual void SetSimulationCommandCallback(void(*sc)(const char*, const char*, void*), void* scdata) = 0;


    virtual ~LibSimModuleInterface();

private:

};






#endif // !MODULEINTERFACE_H
