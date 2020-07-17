#include "exeption.h"
#include "metaData.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace vistle::insitu::sensei;

MetaData::ConstMeshIter vistle::insitu::sensei::MetaData::getMesh(const std::string& name)const
{
	return m_meshes.find(name);
}

MetaData::MeshIter vistle::insitu::sensei::MetaData::addMesh(const std::string& name)
{
	return m_meshes.insert({ name, std::vector<std::string>() }).first;
}

MetaData::MeshIter vistle::insitu::sensei::MetaData::getMesh(const std::string& name)
{
	return m_meshes.find(name);
}

MetaData::ConstMeshIter vistle::insitu::sensei::MetaData::begin() const
{
	return m_meshes.begin();
}

MetaData::ConstMeshIter vistle::insitu::sensei::MetaData::end() const
{
	return m_meshes.end();
}

MetaData::MeshIter vistle::insitu::sensei::MetaData::begin() 
{
	return m_meshes.begin();
}

MetaData::MeshIter vistle::insitu::sensei::MetaData::end() 
{
	return m_meshes.end();
}

void vistle::insitu::sensei::MetaData::addVariable(const std::string& varName, const MeshIter& mesh)
{
	if (mesh != end())
	{
		mesh->second.push_back(varName);
	}
	else
	{
		std::cerr << "MetaData can not add variable " << varName << " to mesh: mesh not found!" << std::endl;
	}
}

void vistle::insitu::sensei::MetaData::addVariable(const std::string& varName, const std::string& meshName)
{
	m_meshes[meshName].push_back(varName);
}

MetaData::MetaVar vistle::insitu::sensei::MetaData::getVariables(const MetaData::ConstMeshIter& mesh) const
{

	if (mesh != end())
	{
		return MetaVar{ mesh->second.begin(), mesh->second.end() };
	}
	else
	{
		throw InsituExeption{} << "MetaData::getVariables can not find mesh!";
	}
}

MetaData::MetaVar vistle::insitu::sensei::MetaData::getVariables(const std::string& mesh) const
{
	return getVariables(getMesh(mesh));
}

MetaData::VarIter vistle::insitu::sensei::MetaData::MetaVar::begin()
{
	return m_begin;
}

MetaData::VarIter vistle::insitu::sensei::MetaData::MetaVar::end()
{
	return m_end;
}