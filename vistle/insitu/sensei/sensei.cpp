#include "sensei.h"
#include "exeption.h"

#include <vistle/util/hostname.h>

#include <iostream>
#include <fstream>
#include <cassert>

#include <boost/mpi/collectives.hpp>

#define CERR std::cerr << "[" << m_rank << "/" << m_mpiSize << " ] SenseiAdapter: "
using std::endl;
using namespace vistle::insitu;
using namespace vistle::insitu::sensei;
using namespace vistle::insitu::message;
SenseiAdapter::SenseiAdapter(bool paused, MPI_Comm Comm, MetaData&& meta, Callbacks cbs)
	:m_callbacks(cbs)
	, m_metaData(std::move(meta))
{
	MPI_Comm_rank(Comm, &m_rank);
	MPI_Comm_size(Comm, &m_mpiSize);
	m_commands["run/paused"] = !paused; //if true run else wait
	m_commands["exit"] = false; //let the simulation know that vistle wants to exit by returning false from execute
	try
	{
		m_messageHandler.initialize(m_rank);
		dumpConnectionFile(comm);
	}
	catch (...)
	{
		throw vistle::insitu::sensei::Exeption() << "failed to create connection facilities for Vistle";
	}

	m_intOptions[message::InSituMessageType::KeepTimesteps] = std::unique_ptr< IntOption<message::KeepTimesteps>>(new IntOption<message::KeepTimesteps>{ true });
	m_intOptions[message::InSituMessageType::NthTimestep] = std::unique_ptr<IntOption<message::NthTimestep>>(new IntOption<message::NthTimestep>{1});

}

bool SenseiAdapter::Execute(size_t timestep)
{
	bool wasConnected = m_connected;
	while (recvAndHandeMessage()) {} //catch newest state
	if (wasConnected && !m_connected)
	{
		CERR << "sensei controller disconnected" << endl;
		return false;
	}
	if (m_commands["exit"])
	{
		m_messageHandler.send(insitu::message::ConnectionClosed{ true });
		return false;
	}
	auto it = m_commands.find("run/paused");
	while (!it->second)//also let the simulation wait for the module if initialized with paused
	{
		recvAndHandeMessage(true);
		if (m_commands["exit"])
		{
			m_messageHandler.send(insitu::message::ConnectionClosed{ true });
			return false;
		}
		if (!m_connected)
		{
			CERR << "sensei controller disconnected" << endl;
			return false;
		}
	}

	if (!m_moduleInfo.ready)
	{
		CERR << "can not process data if the sensei controller is not executing!" << endl;
		return true;
	}
	if (timestep % m_intOptions[message::InSituMessageType::NthTimestep]->val != 0) {
        return true;
    }
	auto dataObjects = m_callbacks.getData(m_usedData);
	for (const auto dataObject : dataObjects)
	{
		if (m_intOptions[message::InSituMessageType::KeepTimesteps]->val) {
			dataObject.object()->setTimestep(m_timestep);
		}
		else {
			dataObject.object()->setIteration(m_timestep);
		}
		addObject(dataObject.portName(), dataObject.object());
	}
	++m_timestep;
	return true;
}

bool vistle::insitu::sensei::SenseiAdapter::Finalize()
{
	CERR << "Finalizing" << endl;
	if (m_moduleInfo.initialized)
	{
		m_messageHandler.send(vistle::insitu::message::ConnectionClosed{ true });
	}
	return true;
}

bool vistle::insitu::sensei::SenseiAdapter::processTimestep(size_t timestep)
{
	return false;
}

SenseiAdapter::~SenseiAdapter()
{
}


void vistle::insitu::sensei::SenseiAdapter::calculateUsedData()
{
	m_usedData = MetaData{};
	for (auto simMesh = m_metaData.begin(); simMesh != m_metaData.end(); ++simMesh)
	{
		for (const auto &connectedData : m_moduleInfo.connectedPorts)
		{
			if (connectedData == simMesh->first) //the mesh is connected directly
			{
				m_usedData.addMesh(simMesh->first);
			}
			else
			{
				for (const auto& varName : m_metaData.getVariables(simMesh))
				{
					if (connectedData == simMesh->first + "_" + varName)
					{
						m_usedData.addVariable(varName, simMesh->first);
						break;
					}
				}
				
				auto it = std::find(m_metaData.getVariables(simMesh).begin(), m_metaData.getVariables(simMesh).end(), connectedData); //find out if a variable of this mesh is connected
				if (it != m_metaData.getVariables(simMesh).end())
				{
					m_usedData.addVariable(*it, simMesh->first);
				}
			}
		}
	}
}

void SenseiAdapter::dumpConnectionFile(MPI_Comm Comm)
{
	
	std::vector<std::string> names;
	boost::mpi::gather(boost::mpi::communicator(comm, boost::mpi::comm_attach), m_messageHandler.name(), names, 0);
	if (m_rank == 0)
	{
		std::ofstream outfile("sensei.vistle");
		for (int i = 0; i < m_mpiSize; i++)
		{
			outfile << std::to_string(i) << " " << names[i] << endl;
		}
		outfile.close();

	}

}

bool SenseiAdapter::recvAndHandeMessage(bool blocking)
{
	message::Message msg = blocking ? m_messageHandler.recv() : m_messageHandler.tryRecv();
	if (msg.type() != insitu::message::InSituMessageType::Invalid)
	{
		std::cerr << "received message of type " << static_cast<int>(msg.type()) << std::endl;
	}
	
	switch (msg.type())
	{
	case insitu::message::InSituMessageType::Invalid:
		return false;
	case insitu::message::InSituMessageType::ShmInit:
	{
		if (m_connected)
		{
			CERR << "warning: received connection attempt but we are already connectted with module " << m_moduleInfo.name << m_moduleInfo.id << endl;
			return false;
		}
		message::ShmInit init = msg.unpackOrCast<message::ShmInit>();
		//vector<string> args{ to_string(size()), vistle::Shm::the().instanceName(), name(), to_string(id()), vistle::hostname(), to_string(InstanceNum()) };
		std::vector<std::string>& argV = init.value;
		int mpisize = std::stoi(argV[0]);
		if (m_mpiSize != mpisize)
		{
			CERR << "Vistle's mpi = " << mpisize << " and this mpi size = " << m_mpiSize << " do not match" << endl;
			m_messageHandler.send(ConnectionClosed{true});
			return false;
		}
		m_moduleInfo.shmName = argV[1];
		m_moduleInfo.name = argV[2];
		m_moduleInfo.id = std::stoi(argV[3]);
		m_moduleInfo.hostname = argV[4];
		m_moduleInfo.numCons = argV[5];
		if (m_rank == 0 && argV[4] != vistle::hostname())
		{
			CERR << "this " << vistle::hostname() << "trying to connect to " << argV[4] << endl;
			CERR << "Wrong host: must connect to Vistle on the same machine!" << endl;
			m_messageHandler.send(ConnectionClosed{true});
			return false;
		}
		if (!initializeVistleEnv())
		{
			m_messageHandler.send(ConnectionClosed{true});
			return false;
		}
		m_moduleInfo.initialized = true;
		std::vector<std::string> commands;
		for (const auto& command : m_commands)
		{
			commands.push_back(command.first);
		}
		m_messageHandler.send(SetCommands{ commands });
		addPorts();
	}
	break;
	case insitu::message::InSituMessageType::AddObject:
		break;
	case insitu::message::InSituMessageType::SetPorts:
	{
		auto m = msg.unpackOrCast<message::SetPorts>();
		for (auto portList : m.value)
		{
			for (auto port : portList)
			{
				m_moduleInfo.connectedPorts.insert(port);
			}
		}
		calculateUsedData();
	}
	break;
	case insitu::message::InSituMessageType::SetCommands:
		break;
	case insitu::message::InSituMessageType::Ready:
	{
		m_timestep = 0;
		Ready em = msg.unpackOrCast<Ready>();
		m_moduleInfo.ready = em.value;
		if (m_moduleInfo.ready) {
			vistle::Shm::the().setObjectID(m_shmIDs.objectID());
			vistle::Shm::the().setArrayID(m_shmIDs.arrayID());
		}
		else {
			m_shmIDs.set(vistle::Shm::the().objectID(), vistle::Shm::the().arrayID());
			m_messageHandler.send(Ready{ false }); //confirm that we are done creating vistle objects
		}
	}
	break;
	case insitu::message::InSituMessageType::ExecuteCommand:
	{
		ExecuteCommand exe = msg.unpackOrCast<ExecuteCommand>();
		auto it = m_commands.find(exe.value);
		if (it != m_commands.end())
		{
			it->second = !it->second;
		}
		else
		{
			CERR << "receive unknow command: " << exe.value << endl;
		}

	}
	break;
	case insitu::message::InSituMessageType::GoOn:
		break;
	case insitu::message::InSituMessageType::ConstGrids:
		break;
	case insitu::message::InSituMessageType::ConnectionClosed:
	{
		m_sendMessageQueue.reset(nullptr);
		m_connected = false;
		m_shmIDs.close();
		CERR << "connection closed" << endl;
	}
	break;
	case insitu::message::InSituMessageType::VTKVariables:
		break;
	case insitu::message::InSituMessageType::CombineGrids:
		break;
	break;
	default:
        m_intOptions[msg.type()]->setVal(msg); //KeepTimesteps, NthTimestep
		break;
	}
	return true;
}

bool SenseiAdapter::initializeVistleEnv()
{
	vistle::registerTypes();
#ifdef SHMPERRANK
	//remove the"_r" + std::to_string(rank) because that gets added by attach again
	char c_ = ' ', cr = ' ';
	while (m_moduleInfo.shmName.size() > 0 && c_ != '_' && cr != 'r')
	{
		cr = c_;
		c_ = m_moduleInfo.shmName[m_moduleInfo.shmName.size() - 1];
		m_moduleInfo.shmName.pop_back();
	}
	if (m_moduleInfo.shmName.size() == 0)
	{
		CERR << "failed to create shmName: expected it to end with _r + std::to_string(rank)" << endl;
		return false;
	}
#endif // SHMPERRANK


	try
	{
		CERR << "attaching to shm: name = " << m_moduleInfo.shmName << " id = " << m_moduleInfo.id << " rank = " << m_rank << endl;
		vistle::Shm::attach(m_moduleInfo.shmName, m_moduleInfo.id, m_rank);
	}
	catch (const vistle::shm_exception& ex)
	{
		CERR << ex.what() << endl;
		return false;
	}
	try {
		m_sendMessageQueue.reset(new AddObjectMsq(m_moduleInfo, m_rank));
	}
	catch (const InsituExeption& ex) {
		CERR << "failed to create AddObjectMsq numCons =  " << m_moduleInfo.numCons << " module id = " << m_moduleInfo.id << " : " << ex.what() << endl;
		return false;
	}


	try {
		m_shmIDs.initialize(m_moduleInfo.id, m_rank, std::stoi(m_moduleInfo.numCons), insitu::message::SyncShmIDs::Mode::Attach);
	}
	catch (const vistle::exception&) {
		CERR << "failed to initialize SyncShmMessage" << endl;
		return false;
	}

	m_connected = true;
	CERR << "connection to module " << m_moduleInfo.name << m_moduleInfo.id << " established!" << endl;
	return true;
}

void SenseiAdapter::addPorts()
{
	std::vector<std::vector<std::string>> ports;
	std::vector<std::string> meshNames;
	std::vector<std::string> varNames;
	for (const auto& mesh : m_metaData)
	{
		meshNames.push_back(mesh.first);
		for (const auto& var : mesh.second)
		{
			varNames.push_back(mesh.first + "_" + var);
		}
	}

	meshNames.push_back("mesh");
	ports.push_back(meshNames);
		
	varNames.push_back("variable");
	ports.push_back(varNames);

	m_messageHandler.send(SetPorts{ ports });
}

void SenseiAdapter::addObject(const std::string& port, vistle::Object::const_ptr obj)
{
	if (m_sendMessageQueue)
	{
		m_sendMessageQueue->addObject(port, obj);
	}
	else
	{
		CERR << "VistleSenseiAdapter can not add vistle object: sendMessageQueue = null" << endl;
	}
}