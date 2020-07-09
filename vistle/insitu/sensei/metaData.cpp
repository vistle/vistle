#include "exeption.h"
#include "metaData.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace vistle::insitu;

MetaData::MeshIter vistle::insitu::MetaData::addMesh(const std::string& name)
{
	m_variables.push_back(std::vector<std::string>());
	return m_meshes.emplace(end(), name);
}

MetaData::MeshIter vistle::insitu::MetaData::getMesh(const std::string& name)
{
	return std::find(begin(), end(), name);
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
		m_variables[mesh - begin()].push_back(varName);
	}
	else
	{
		std::cerr << "MetaData can not add variable " << varName << " to mesh: mesh not found!" << std::endl;
	}
}

void vistle::insitu::MetaData::addVariable(const std::string& varName, const std::string& meshName)
{
	addVariable(varName, getMesh(meshName));
}

MetaData::MetaVar vistle::insitu::MetaData::getVariables(const MeshIter& mesh)
{

	if (mesh != end())
	{
		auto& vars = m_variables[mesh - begin()];
		return MetaVar{ vars.begin(), vars.end() };
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