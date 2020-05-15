#include "sensei.h"

#include "rectilinerGrid.h"
#include "unstructuredGrid.h"
#include "structuredGrid.h"

#include <insitu/core/dataAdaptor.h>
#include <insitu/core/exeption.h>

#include <util/hostname.h>

#include <iostream>
#include <fstream>

#define CERR std::cerr << "[" << m_rank << "/" << m_mpiSize << " ] SenseiAdapter: "
using std::endl;
using namespace insitu;
using namespace insitu::message;
bool insitu::sensei::SenseiAdapter::Initialize(bool paused, size_t rank, size_t mpiSize, MetaData&& meta, Callbacks cbs, ModuleInfo moduleInfo)
{
	m_callbacks = cbs;
	m_metaData = std::move(meta);
	m_moduleInfo = moduleInfo;
	m_rank = rank;
	m_mpiSize = mpiSize;
	
	try
	{
		m_messageHandler.initialize();
		dumpConnectionFile();
	}
	catch (...)
	{
		return false;
	}
	if (paused) //let the simulation wait for the module
	{
		while (!m_moduleInfo.ready)
		{
			recvAndHandeMessage(true);
		}
	}
	return true;
}

bool insitu::sensei::SenseiAdapter::Execute(size_t timestep)
{
	m_currTimestep = timestep;
	recvAndHandeMessage(); //catch newest state

	for (std::string name : m_moduleInfo.connectedPorts)
	{
		auto it = std::find(m_metaData.meshes.begin(), m_metaData.meshes.end(), name);
		if (it != m_metaData.meshes.end())
		{
			auto mesh = m_callbacks.getMesh(name);
			sendMeshToModule(mesh);
		}
	}
	return false;
}

insitu::sensei::SenseiAdapter::~SenseiAdapter()
{
}


void insitu::sensei::SenseiAdapter::dumpConnectionFile()
{
	std::ofstream outfile("sensei.vistle");
	outfile << m_messageHandler.name() << endl;
	outfile.close();
}

bool insitu::sensei::SenseiAdapter::recvAndHandeMessage(bool blocking)
{
	message::Message msg = blocking ? m_messageHandler.recv() : m_messageHandler.tryRecv();
	switch (msg.type())
	{
	case insitu::message::InSituMessageType::Invalid:
		return false;
	case insitu::message::InSituMessageType::ShmInit:
	{
		if (m_connected)
		{
			CERR << "warning: received conncetion attempt but we are already connectted with module " << m_moduleInfo.name << m_moduleInfo.id << endl;

		}
		message::ShmInit init = msg.unpackOrCast<message::ShmInit>();
		//vector<string> args{ to_string(size()), vistle::Shm::the().instanceName(), name(), to_string(id()), vistle::hostname(), to_string(InstanceNum()) };
		std::vector<std::string>& argV = init.value;
		int mpisize = std::stoi(argV[0]);
		if (m_mpiSize != mpisize)
		{
			CERR << "Vistle's mpi = " << mpisize << " and this mpi size = " << m_mpiSize << " do not match" << endl;
		}
		m_moduleInfo.shmName = argV[1];
		m_moduleInfo.name = argV[2];
		m_moduleInfo.id = std::stoi(argV[3]);
		m_moduleInfo.hostname = argV[4];
		m_moduleInfo.numCons = argV[6];
		if (m_rank == 0 && argV[4] != vistle::hostname()) {
			CERR << "this " << vistle::hostname() << "trying to connect to " << argV[4] << endl;
			CERR << "Wrong host: must connect to Vistle on the same machine!" << endl;
			return false;
		}
		initializeVistleEnv();
		m_messageHandler.send(SetCommands{ std::vector<std::string>{"run", "stop"} });
		m_commands["run"] = true;
		m_commands["stop"] = false;
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
				m_moduleInfo.connectedPorts.push_back(port);
			}
		}
	}
	break;
	case insitu::message::InSituMessageType::SetCommands:
		break;
	case insitu::message::InSituMessageType::Ready:
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
	case insitu::message::InSituMessageType::NthTimestep:
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
	case insitu::message::InSituMessageType::KeepTimesteps:
		break;
	default:
		CERR << "received message with unknown type " << static_cast<int>(msg.type()) << endl;
		return false;
		break;
	}
	return true;
}

bool insitu::sensei::SenseiAdapter::initializeVistleEnv()
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
	catch (InsituExeption& ex) {
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

}

void insitu::sensei::SenseiAdapter::addPorts()
{
	SetPorts::value_type ports;
	auto meshNames = m_metaData.meshes;

	meshNames.push_back("mesh");
	ports.push_back(meshNames);

	std::vector<std::string> varNames;
	for (auto name : m_metaData.variables)
	{
		varNames.push_back(name.first);
	}
	varNames.push_back("variable");
	ports.push_back(varNames);

	std::vector<std::string> debugNames;
	int i = 0;
	for (auto mesh : meshNames)
	{
		debugNames.push_back(mesh + "_mpi_ranks");
	}

	debugNames.push_back("debug");
	ports.push_back(debugNames);

	m_messageHandler.send(SetPorts{ ports });
}



void insitu::sensei::SenseiAdapter::sendMeshToModule(const Mesh& mesh)
{
	switch (mesh.type)
	{
	case vistle::Object::PLACEHOLDER:
	{
		auto multiMesh = dynamic_cast<const MultiMesh*>(&mesh);
		if (multiMesh)
		{
			for (auto m : multiMesh->meshes)
			{
				sendMeshToModule(m);
			}
		}
	}
		break;
		case vistle::Object::UNSTRUCTUREDGRID:
	{
		auto unstr = dynamic_cast<const UnstructuredMesh*>(&mesh);
		if (unstr)
		{
			auto m = makeUnstructuredGrid(*unstr, m_shmIDs, m_currTimestep);
			sendObject(mesh.name, m);
		}
		else
		{
			CERR << "failed dynamic_cast: mesh of type UNSTRUCTUREDGRID is expected to be a UnstructuredMesh" << endl;
		}

	}
		break;
	case vistle::Object::RECTILINEARGRID:
	{
		auto m = makeRectilinearGrid(mesh, m_shmIDs, m_currTimestep);
		sendObject(mesh.name, m);
	}
		break;
	case vistle::Object::STRUCTUREDGRID:
	{
		auto m = makeStructuredGrid(mesh, m_shmIDs, m_currTimestep);
		sendObject(mesh.name, m);
	}
		break;
	default:
		CERR << "sendMeshToModule: mesh type " << mesh.type << " not implemented!" << endl;
		break;
	}

}

void insitu::sensei::SenseiAdapter::sendVariableToModule(const Array& var)
{
}

void insitu::sensei::SenseiAdapter::sendObject(const std::string& port, vistle::Object::const_ptr obj)
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

sensei::Callbacks::Callbacks(insitu::Mesh(*getMesh)(const std::string&), insitu::Array(*getVar)(const std::string&))
	:m_getMesh(getMesh)
	,m_getVar(getVar)
{
	
}

Mesh insitu::sensei::Callbacks::getMesh(const std::string& name)
{
	return m_getMesh(name);
}

Array insitu::sensei::Callbacks::getVar(const std::string& name)
{
	return m_getVar(name);
}
