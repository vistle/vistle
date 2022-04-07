#include "metaData.h"
#include "exception.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace vistle::insitu::sensei;

MetaMesh::MetaMesh(const std::string &name): m_name(name)
{}

bool MetaMesh::operator==(const MetaMesh &other) const
{
    return m_name == other.name();
}

bool MetaMesh::operator<(const MetaMesh &other) const
{
    return m_name < other.name();
}

MetaMesh::Iter MetaMesh::begin() const
{
    return m_variables.begin();
}
MetaMesh::Iter MetaMesh::end() const
{
    return m_variables.end();
}
MetaMesh::Iter MetaMesh::addVar(const std::string &name)
{
    return m_variables.insert(end(), name);
}

const std::string &MetaMesh::name() const
{
    return m_name;
}

bool MetaMesh::empty() const
{
    return m_variables.empty();
}

MetaData::Iter MetaData::getMesh(const std::string &name) const
{
    return std::find_if(begin(), end(), [name](const MetaMesh &mesh) { return mesh.name() == name; });
}

MetaData::Iter MetaData::addMesh(const MetaMesh &mesh)
{
    return m_meshes.insert(mesh).first;
}

MetaData::Iter MetaData::begin() const
{
    return m_meshes.begin();
}

MetaData::Iter MetaData::end() const
{
    return m_meshes.end();
}
