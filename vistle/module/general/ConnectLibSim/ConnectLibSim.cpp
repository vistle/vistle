#include "ConnectLibSim.h"

#include <vector>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <fstream>
#include <core/tcpmessage.h>
#include "Engine.h"

#include <util/sleep.h>
#include "EstablishConnection.h"

using namespace vistle;
using std::string;
using std::cerr; using std::endl;
namespace asio = boost::asio;
using namespace in_situ;

ConnectLibSim::ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm)
    : vistle::Module("Connect to a simulation that implements the LibSim in-situ interface", name, moduleID, comm)
{
    if (Engine::createEngine()->isInitialized()) {
        Engine::createEngine()->setModule(this);
        Engine::createEngine()->SetTimestepChangedCb(std::bind(&ConnectLibSim::timestepChanged, this));
        Engine::createEngine()->setDoReadMutex(&isReadingMutex);
        Engine::createEngine()->SetDisconnectCb([this]() {
           std::lock_guard<std::mutex> g(isReadingMutex);
           isReading = false;
           });
       std::lock_guard<std::mutex> g(isReadingMutex);
       isReading = true;
    }
    int size = -1;
    MPI_Comm_size(comm, &size);
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


bool ConnectLibSim::prepare() {
    isReadingMutex.lock();
    isReading = true;
    isReadingMutex.unlock();

    while (true) {
        adaptive_wait(false, this);
        std::lock_guard<std::mutex> g(isReadingMutex);
        if (cancelRequested(true)) {
            isReading = false;
            break;
        }

        if (!isReading) {
            break;
        }

    }
    return true;
    
}







MODULE_MAIN(ConnectLibSim)





