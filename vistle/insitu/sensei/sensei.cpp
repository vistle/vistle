#include "sensei.h"
#include <insitu/core/dataAdaptor.h>

#include <iostream>
#include <fstream>
#define CERR std::cerr << "SenseiAdapter: "
using std::endl;
using namespace insitu;
bool insitu::sensei::SenseiAdapter::Initialize(MetaData&& meta, Callbacks cbs)
{
	m_callbacks = cbs;
	m_metaData = std::move(meta);

	try
	{
		m_messageHandler.initialize();
		dumpConnectionFile();
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool insitu::sensei::SenseiAdapter::Execute()
{
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
	delete m_sendMessageQueue;
}


void insitu::sensei::SenseiAdapter::dumpConnectionFile()
{
	std::ofstream outfile("sensei.vistle");
	outfile << m_messageHandler.name() << endl;
	outfile.close();
}

void insitu::sensei::SenseiAdapter::recvAndHandeMessage()
{
	while (true)
	{
		auto msg = m_messageHandler.tryRecv();
		switch (msg.type())
		{
		case insitu::message::InSituMessageType::Invalid:
			return;
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
			break;
		}
	}
}

void insitu::sensei::SenseiAdapter::sendMeshToModule(const Mesh& mesh)
{
	switch (mesh.type)
	{
	case vistle::Object::COORD:
		break;
	case vistle::Object::COORDWRADIUS:
		break;
	case vistle::Object::DATABASE:
		break;
	case vistle::Object::UNKNOWN:
		break;
	case vistle::Object::EMPTY:
		break;
	case vistle::Object::PLACEHOLDER:
		break;
	case vistle::Object::TEXTURE1D:
		break;
	case vistle::Object::POINTS:
		break;
	case vistle::Object::SPHERES:
		break;
	case vistle::Object::LINES:
		break;
	case vistle::Object::TUBES:
		break;
	case vistle::Object::TRIANGLES:
		break;
	case vistle::Object::POLYGONS:
		break;
	case vistle::Object::UNSTRUCTUREDGRID:
		break;
	case vistle::Object::UNIFORMGRID:
		break;
	case vistle::Object::RECTILINEARGRID:
		break;
	case vistle::Object::STRUCTUREDGRID:
		break;
	case vistle::Object::QUADS:
		break;
	case vistle::Object::VERTEXOWNERLIST:
		break;
	case vistle::Object::CELLTREE1:
		break;
	case vistle::Object::CELLTREE2:
		break;
	case vistle::Object::CELLTREE3:
		break;
	case vistle::Object::NORMALS:
		break;
	case vistle::Object::VEC:
		break;
	default:
		break;
	}

}

void insitu::sensei::SenseiAdapter::sendVariableToModule(const Array& var)
{
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
