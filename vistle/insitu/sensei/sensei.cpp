#include "sensei.h"
#include <insitu/core/dataAdaptor.h>

#include <iostream>
#include <fstream>

#include <insitu/core/exeption.h>
#include "rectilinerGrid.h"
#include "unstructuredGrid.h"
#include "structuredGrid.h"

#define CERR std::cerr << "SenseiAdapter: "
using std::endl;
using namespace insitu;
bool insitu::sensei::SenseiAdapter::Initialize(bool paused, size_t rank, size_t mpiSize, MetaData&& meta, Callbacks cbs, ModuleInfo moduleInfo)
{
	m_callbacks = cbs;
	m_metaData = std::move(meta);
	m_moduleInfo = moduleInfo;
	m_rank = rank;
	m_mpiSize = mpiSize;
	try
	{
		m_sendMessageQueue.reset(new AddObjectMsq(m_moduleInfo, m_rank));
	}
	catch (const InsituExeption& ex)
	{
		CERR << "opening send message queue: " << ex.what() << endl;
		return false;
	}

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
		break;
	case insitu::message::InSituMessageType::GoOn:
		break;
	case insitu::message::InSituMessageType::ConstGrids:
		break;
	case insitu::message::InSituMessageType::NthTimestep:
		break;
	case insitu::message::InSituMessageType::ConnectionClosed:
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
