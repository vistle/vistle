#ifndef VISTLE_INSITU_CATALYST2_CONDUITTOPOLOGY_H
#define VISTLE_INSITU_CATALYST2_CONDUITTOPOLOGY_H

#include <catalyst_conduit_blueprint.hpp>
#include <catalyst_conduit.hpp>
#include <map>
#include <memory>
#include <string>
#include <vistle/core/unstr.h>

struct ConduitArray {
    const conduit_int32 *data = nullptr;
    conduit_index_t size = 0;
    conduit_int32 operator[](size_t i) const;
};

struct ConduitTopology {
    ConduitTopology(const conduit_cpp::Node &mesh);
    const ConduitArray connectivity;
    const vistle::UnstructuredGrid::Type shape;
    const ConduitArray shapes;
    const std::map<int, vistle::UnstructuredGrid::Type> shapeMap;
    const ConduitArray sizes;
    const ConduitArray offsets;
    const bool isMixed = false;

    const std::unique_ptr<ConduitTopology> sub;

private:
    ConduitTopology(const conduit_cpp::Node &mesh, const conduit_cpp::Node &meshElements);
    std::unique_ptr<ConduitTopology> getSubTopologyIfAvailable(const conduit_cpp::Node &node, const std::string &key);
};

#endif // VISTLE_INSITU_CATALYST2_CONDUITTOPOLOGY_H
