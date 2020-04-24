#ifndef VISTLE_SENSEI_H
#define VISTLE_SENSEI_H

#include "export.h"

#include <mpi.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

#include <insitu/message/SyncShmIDs.h>
#include <insitu/message/ShmMessage.h>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <insitu/core/moduleInfo.h>
#include <insitu/core/dataAdaptor.h>

namespace vistle {
namespace message {
class  MessageQueue;
}
}

namespace insitu {
namespace sensei{
	class DataAdapter;
	class Callbacks {
	public:
		Callbacks(Mesh(*getMesh)(const std::string&), Array(*getVar)(const std::string&));
		Mesh getMesh(const std::string& name);
		Array getVar(const std::string& name);
	private:
		Mesh(*m_getMesh)(const std::string&) = nullptr;
		Array(*m_getVar)(const std::string&) = nullptr;
	};
	class V_SENSEIEXPORT SenseiAdapter
	{
	public:

		bool Initialize(MetaData &&meta, Callbacks cbs);
		bool Execute();
		bool Finalize();

		bool processTimestep(size_t timestep); //true if we have sth to do for the given timestep


		void operator =(SenseiAdapter&& other) = delete;
		void operator =(SenseiAdapter& other) = delete;
		SenseiAdapter(SenseiAdapter&& other) = delete;
		SenseiAdapter(SenseiAdapter& other) = delete;
		~SenseiAdapter();



	private:
		//unsigned int m_rank = 0, m_mpiSize = 0;
		Callbacks m_callbacks;
		MetaData m_metaData;
		ModuleInfo m_moduleInfo;
		bool m_initialized = false; //Engine is initialized
        vistle::message::MessageQueue* m_sendMessageQueue = nullptr; //Queue to send addObject messages to LibSImController module
        //mpi info
        int m_rank = -1, m_mpiSize = 0;
        MPI_Comm comm = MPI_COMM_WORLD;
		
        insitu::message::SyncShmIDs m_shmIDs;

		message::InSituShmMessage m_messageHandler;



		bool m_ready = false; //true if the module is connected and executing
		
		void startListening(); //wait for the senei module to connect
		void dumpConnectionFile(); //create a file in which the sensei module can find the connection info
		void updateStatus();
		void recvAndHandeMessage();
		void sendMeshToModule(const Mesh& mesh);
		void sendVariableToModule(const Array& var);

	};
}
}




#endif // !VISTLE_SENSEI_H
