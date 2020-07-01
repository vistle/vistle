#ifndef VISTLE_INSITU_DATAADAPTORTOVISTLE_H
#define VISTLE_INSITU_DATAADAPTORTOVISTLE_H
#include "export.h"


namespace vistle {
namespace message {
	class  MessageQueue;
} //vistle::message


namespace insitu
{
	namespace message {
		class SyncShmIDs;
	} //vistle::insitu::message

	//void V_INSITUCOREEXPORT createVistleObjects(const DataAdaptor* data, message::SyncShmIDs* sync, vistle::message::MessageQueue* sender);
}//vistle::insitu
}//vistle



#endif // !VISTLE_INSITU_DATAADAPTORTOVISTLE_H


