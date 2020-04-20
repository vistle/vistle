#ifndef VISTLE_SENSEI_H
#define VISTLE_SENSEI_H

#include "export.h"

#include <mpi.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

#include <insitu/message/InSituMessage.h>

#include <boost/interprocess/ipc/message_queue.hpp>

namespace vistle {
namespace message {
class  MessageQueue;
}
}

namespace insitu {
namespace sensei{
	class DataAdapter;
	
	class V_SENSEIEXPORT SenseiAdapter
	{
	public:
		bool Initialize();
		bool Execute(const DataAdapter* dataAdapter);
		bool Finalize();

		bool processTimestep(size_t timestep); //true if we have sth to do for the given timestep


		void operator =(SenseiAdapter&& other) = delete;
		void operator =(SenseiAdapter& other) = delete;
		SenseiAdapter(SenseiAdapter&& other) = delete;
		SenseiAdapter(SenseiAdapter& other) = delete;
		~SenseiAdapter();
	private:
        bool m_initialized = false; //Engine is initialized
		bool m_isConnected = false;
		bool m_moduleConnected = false;
        vistle::message::MessageQueue* m_sendMessageQueue = nullptr; //Queue to send addObject messages to LibSImController module
        //mpi info
        int m_rank = -1, m_mpiSize = 0;
        MPI_Comm comm = MPI_COMM_WORLD;

        insitu::message::SyncShmIDs m_shmIDs;


		boost::interprocess::message_queue sendMsq, recvMsq; //to exchange command with the module



		bool m_ready = false; //true if the module is connected and executing
		
		void startListening(); //wait for the senei module to connect
		void dumpConnectionFile(); //create a file in which the sensei module can find the connection info
		void updateStatus();
		void recvAndHandeMessage();

	};
}
}




#endif // !VISTLE_SENSEI_H
