#include "ConnectLibSim.h"

#include <vector>
#include <memory>


#include <fstream>
#include <core/tcpmessage.h>
#include "Engine.h"

#include <util/sleep.h>
#include <insitu/LibSim/EstablishConnection.h>



using namespace vistle;
using std::string;
using std::cerr; using std::endl;

using namespace in_situ;

ConnectLibSim::ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm)
    : vistle::Module("Connect to a simulation that implements the LibSim in-situ interface", name, moduleID, comm)
{
    runMode = addIntParameter("runMode", "change what happens on execute with the simulation", keepRunning, Parameter::Choice);
    V_ENUM_SET_CHOICES(runMode, RunMode);
    consistentMesh = addIntParameter("consistent mesh", "is the mesh consistent over the timesteps/iterations of the simulation", true, Parameter::Boolean);
    nThTimestep = addIntParameter("frequency", "collect data for ever n-th timestep", 1);
    setParameterRange(nThTimestep, (Integer)1, (Integer)0x1000);


    fakeInputPort = createInputPort("fake");


}

int ConnectLibSim::updateParameter(const char* info) {
    constexpr int maxNameSize = 100;
    char name[maxNameSize];
    sscanf(info, "%s:", name);
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
        std::vector<char> val(maxNameSize);
        sscanf(info, "%s:%d", val.data(), &enabled);
        setParameter(name, std::string(val.data()));
    }
    break;
    case vistle::Parameter::Type::Vector:
    {
        vistle::ParamVector val;
        sscanf(info, "%lf,%lf,%lf:%d", &val[0], &val[1], &val[2], &enabled);
        setParameter(name, val);
    }
    break;
    default:
        return false;
        break;
    }
    return true;
}

ConnectLibSim::~ConnectLibSim() {
}

bool ConnectLibSim::timestepChanged() {
    metaData_.timestepChanged = true;
    return isReading;
}
#ifdef MODULE_TREAD
std::mutex* ConnectLibSim::getIsExecutingMutex() {
    return &isExecutingMutex;
}
#endif // MODULE_TREAD



void ConnectLibSim::disconnect() {
#ifdef MODULE_TREAD
    std::lock_guard<std::mutex> g(isExecutingMutex);
#endif // MODULE_TREAD

    isReading = false;
}

bool ConnectLibSim::prepare() {
    isReading = true;


    //isExecutingMutex.lock();
    //isReading = true;
    //isExecutingMutex.unlock();

    //while (true) {
    //    adaptive_wait(false, this);
    //    std::lock_guard<std::mutex> g(isExecutingMutex);
    //    if (cancelRequested(true)) {
    //        isReading = false;
    //        break;
    //    }

    //    if (!isReading) {
    //        break;
    //    }

    //}
    return true;

}

bool ConnectLibSim::reduce(int timestep) {
    isReading = false;
    return true;
}







MODULE_MAIN(ConnectLibSim)





