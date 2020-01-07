#ifndef CONNECT_LIB_SIM_H
#define CONNECT_LIB_SIM_H

#include <module/module.h>

#include <memory>
#include <string>
#include <map>

#include <boost/asio.hpp>
#include "VisItExports.h"
#include "MetaData.h"


class V_VISITXPORT ConnectLibSim : public vistle::Module
{
public:
    ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm);

    int updateParameter(const char* info);

    ~ConnectLibSim() override;
    //called from the simulation to inform us that the next read has to do sth
    bool timestepChanged();
    
private:

    in_situ::Metadata metaData_;

    std::mutex isReadingMutex;
    bool isReading = false;

    // Inherited via Reader
    virtual bool prepare() override;

};




#endif // !CONNECT_LIB_SIM_H
