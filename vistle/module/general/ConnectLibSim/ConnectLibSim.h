#ifndef CONNECT_LIB_SIM_H
#define CONNECT_LIB_SIM_H

#include <module/reader.h>

#include <memory>
#include <string>
#include <map>

#include <boost/asio.hpp>
#include "export.h"
#include "MetaData.h"

class V_CONNECTLIBSIMEXPORT ConnectLibSim : public vistle::Reader
{
public:
    ConnectLibSim(const std::string& name, int moduleID, mpi::communicator comm);

    int updateParameter(const char* info);

    ~ConnectLibSim() override;
    //called from the simulation to inform us that the next read has to do sth
    bool timestepChanged();
    //Parameter
    vistle::StringParameter* sim2File = nullptr; //file with connection information to initialize the connection with the simulation
    
private:
    in_situ::Metadata metaData_;

    std::mutex doReadMutex;
    bool doRead = false;

    // Inherited via Reader
    virtual bool read(Token& token, int timestep = -1, int block = -1) override;
    virtual bool examine(const vistle::Parameter* param) override;

};




#endif // !CONNECT_LIB_SIM_H
