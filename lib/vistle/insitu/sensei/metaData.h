#ifndef VISTLE_INSITU_SENSEI_METADATA_H
#define VISTLE_INSITU_SENSEI_METADATA_H

#include "export.h"

#include <set>
#include <string>
#include <vector>

namespace vistle {
namespace insitu {
namespace sensei {

class V_SENSEIEXPORT MetaMesh {
public:
    typedef std::set<std::string>::const_iterator Iter;
    MetaMesh(const std::string &name);
    bool operator==(const MetaMesh &other) const;
    bool operator<(const MetaMesh &other) const;
    Iter begin() const;
    Iter end() const;
    Iter addVar(const std::string &name);
    const std::string &name() const;
    bool empty() const;

private:
    const std::string m_name;
    std::set<std::string> m_variables;
};

class V_SENSEIEXPORT
    MetaData { // This contains the names of all meshes and their linked data fields and must be provided before the connection to
    // Vistle
public:
    typedef std::set<MetaMesh>::const_iterator Iter;

    Iter getMesh(const std::string &name) const;
    Iter addMesh(const MetaMesh &mesh);

    Iter begin() const;
    Iter end() const;

private:
    std::set<MetaMesh> m_meshes;
};

} // namespace sensei
} // namespace insitu
} // namespace vistle
#endif
