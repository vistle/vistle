#ifndef VISTLE_INSITU_DATAADAPTORTOVISTLE_H
#define VISTLE_INSITU_DATAADAPTORTOVISTLE_H
#include "dataAdaptor.h"
#include "export.h"
namespace vistle {
namespace message {
class  MessageQueue;
}
}

namespace insitu
{
namespace message {
class SyncShmIDs;
}
	
void V_INSITUCOREEXPORT createVistleObjects(const DataAdaptor* data, message::SyncShmIDs* sync, vistle::message::MessageQueue* sender);
}



#endif // !VISTLE_INSITU_DATAADAPTORTOVISTLE_H


