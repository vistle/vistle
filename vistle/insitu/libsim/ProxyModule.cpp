#include "ProxyModule.h"
#include <boost/mpi.hpp>


#include <core/paramvector.h>
#include <core/object.h>
#include <core/parameter.h>
#include <core/port.h>
#include <core/grid.h>
#include <core/message.h>
#include <core/parametermanager.h>
#include <core/messagesender.h>
#include <core/messagepayload.h>
#include <core/shm.h>
#include <core/messagequeue.h>
using namespace vistle;

insitu::ProxyModule::ProxyModule(const std::string& shmName, const std::string& moduleName, int moduleID) 
:vistle::Module("proxy module", moduleName, moduleID, mpi::communicator())
{
        
}
