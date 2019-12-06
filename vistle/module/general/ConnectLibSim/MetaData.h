#ifndef SIMULATION_META_DATA_H
#define SIMULATION_META_DATA_H

#include "VisItDataTypes.h"
namespace in_situ {
struct Metadata {
    bool timestepChanged = false;
    int  simMode = VISIT_SIMMODE_UNKNOWN;
    int currentCycle = 0;
    double currentTime = 0;
};
}



#endif // !SIMULATION_META_DATA_H
