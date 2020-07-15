#include "exeption.h"
#include "metaData.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace vistle::insitu;

MetaData::MeshIter vistle::insitu::MetaData::addMesh(const std::string& name)
{
	return m_meshes.insert({ name, std::vector<std::string>() }).first;
}

MetaData::MeshIter vistle::insitu::MetaData::getMesh(const std::string& name)
{
	return m_meshes.find(name);
}

MetaData::MeshIter vistle::insitu::MetaData::begin()
{
	return m_meshes.begin();
}

MetaData::MeshIter vistle::insitu::MetaData::end()
{
	return m_meshes.end();
}

void vistle::insitu::MetaData::addVariable(const std::string& varName, const MeshIter& mesh)
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

void vistle::insitu::MetaData::addVariable(const std::string& varName, const std::string& meshName)
{
	m_meshes[meshName].push_back(varName);
}

MetaData::MetaVar vistle::insitu::MetaData::getVariables(const MetaData::MeshIter& mesh)
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

MetaData::MetaVar vistle::insitu::MetaData::getVariables(const std::string& mesh)
{
	return getVariables(getMesh(mesh));
}

MetaData::VarIter vistle::insitu::MetaData::MetaVar::begin()
{
	return m_begin;
}

MetaData::VarIter vistle::insitu::MetaData::MetaVar::end()
{
	return m_end;
}