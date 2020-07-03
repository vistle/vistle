#ifndef VISTLE_SENSEI_H
#define VISTLE_SENSEI_H


#include "export.h"
#include "senseiInterface.h"

#include <mpi.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

#include <insitu/message/SyncShmIDs.h>
#include <insitu/message/ShmMessage.h>
#include <insitu/message/addObjectMsq.h>
#include <boost/interprocess/ipc/message_queue.hpp>


namespace vistle {
namespace message {
class  MessageQueue;
}//message

namespace insitu {
namespace sensei {

class SenseiAdapter : public SenseiInterface

{
public:

	SenseiAdapter(bool paused, size_t rank, size_t mpiSize, MetaData&& meta, Callbacks cbs);
	bool Execute(size_t timestep);
	bool Finalize();

	bool processTimestep(size_t timestep); //true if we have sth to do for the given timestep


	void operator =(SenseiAdapter&& other) = delete;
	void operator =(SenseiAdapter& other) = delete;
	SenseiAdapter(SenseiAdapter&& other) = delete;
	SenseiAdapter(SenseiAdapter& other) = delete;
	~SenseiAdapter();



private:
	size_t m_currTimestep = 0;
	Callbacks m_callbacks;
	MetaData m_metaData;
	ModuleInfo m_moduleInfo;
	struct VariablesUsedByMesh {
		MetaData::MeshIter mesh;
		std::vector<MetaData::VarIter> varNames;//all used variables of mesh
	};
	std::vector<VariablesUsedByMesh> m_usedData;
	bool m_connected = false; //If we are connected to the module
	std::unique_ptr<vistle::insitu::message::AddObjectMsq> m_sendMessageQueue; //Queue to send addObject messages to module
	//mpi info
	int m_rank = -1, m_mpiSize = 0;
	MPI_Comm comm = MPI_COMM_WORLD;

	insitu::message::SyncShmIDs m_shmIDs;

	message::InSituShmMessage m_messageHandler;
	bool m_ready = false; //true if the module is connected and executing
	std::map<std::string, bool> m_commands; //commands and their current state

	void calculateUsedData(); //calculate and store which meshes and variables are requested by Vistle's connected ports
	void dumpConnectionFile(); //create a file in which the sensei module can find the connection info
	bool recvAndHandeMessage(bool blocking = false);
	bool initializeVistleEnv();
	void addPorts();
	void sendMeshToModule(const Grid& grid, const std::vector<Array>& data);
	void sendVariableToModule(const Array& variable, vistle::obj_const_ptr mesh);
	void addObject(const std::string& port, vistle::Object::const_ptr obj);

};
}//sensei
}//insitu
}//vistle



#endif // !VISTLE_SENSEI_H
