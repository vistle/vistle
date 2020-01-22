#ifndef CONNECT_LIB_SIM_H
#define CONNECT_LIB_SIM_H

#include <module/module.h>

#include <memory>
#include <string>
#include <map>

#include <boost/asio.hpp>
#include "VisItExports.h"
#include "MetaData.h"



DEFINE_ENUM_WITH_STRING_CONVERSIONS(RunMode, (singleStep)(keepRunning)(runWhileExecute)(stop))
class V_VISITXPORT ConnectLibSim : public vistle::Module
{
public:
    ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm);

    int updateParameter(const char* info);
    vistle::Port* fakeInputPort = nullptr;
    vistle::IntParameter* runMode = nullptr;
    vistle::IntParameter* consistentMesh = nullptr;
    vistle::IntParameter* nThTimestep = nullptr;
    ~ConnectLibSim() override;
    //called from the simulation to inform us that the next read has to do sth
    bool timestepChanged();
#ifdef MODULE_THREAD
    std::mutex* getIsExecutingMutex();
#endif
    void disconnect();
private:

    in_situ::Metadata metaData_;
#ifdef MODULE_THREAD
    std::mutex isExecutingMutex;
#endif
    bool isReading = false;
    std::unique_ptr<std::thread> readThread;
    // Inherited via Reader

    bool prepare() override;
    bool reduce(int timestep) override;
};




#endif // !CONNECT_LIB_SIM_H
