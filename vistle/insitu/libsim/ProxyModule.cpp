#include "ProxyModule.h"
#include <boost/mpi.hpp>


#include <vistle/core/paramvector.h>
#include <vistle/core/object.h>
#include <vistle/core/parameter.h>
#include <vistle/core/port.h>
#include <vistle/core/grid.h>
#include <vistle/core/message.h>
#include <vistle/core/parametermanager.h>
#include <vistle/core/messagesender.h>
#include <vistle/core/messagepayload.h>
#include <vistle/core/shm.h>
#include <vistle/core/messagequeue.h>
using namespace vistle;

insitu::ProxyModule::ProxyModule(const std::string& shmName, const std::string& moduleName, int moduleID) 
:vistle::Module("proxy module", moduleName, moduleID, mpi::communicator())
{
        
}
