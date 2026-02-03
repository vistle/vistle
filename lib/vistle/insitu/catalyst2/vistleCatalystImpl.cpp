

#include "catalyst_impl_vistle.h"
#include "conduitToVistle.h"

#include <catalyst.h>
#include <conduit.hpp> // for conduit::Node
#include <conduit_cpp_to_c.hpp> // for conduit::cpp_node()

#include <catalyst_api.h>
#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>
#include <catalyst_stub.h>

#include <array>
#include <iostream>
#include <tuple>
#include <utility>
#include <variant>

#include <vistle/insitu/adapter/adapter.h>

#include <vistle/core/unstr.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/uniformgrid.h>


struct ConnectionParams {
    int paused = false;
    int dynamicGrid = false;
    std::string launchOptions;
    MPI_Comm comm;
    int64_t fortranComm = 0;
};

static std::unique_ptr<vistle::insitu::Adapter> adapter = nullptr;
static const conduit_node *currentCNode = nullptr;
static ConnectionParams connectionParams;


template<typename T>
struct ConfigEntry {
    using type = T;
    T *target;
    std::string path;
    T (conduit_cpp::Node::*member)() const = nullptr;
};

template<typename T>
void parseParam(const conduit_cpp::Node &params, T *target, const std::string &path,
                T (conduit_cpp::Node::*member)() const)
{
    try {
        *target = (params[path].*member)();
    } catch (const std::exception &e) {
        std::cerr << "parameter " << path << " is not specified, defaulting to " << *target << "!" << std::endl;
    }
}

template<typename... Types>
void parseParams(const conduit_cpp::Node &params, Types &&...types)
{
    // ((*types.target = (params[types.path].*types.member)()), ...);
    ((parseParam(params, types.target, types.path, types.member)), ...);
}

void parseConnectionParams(const conduit_node *cparams)
{
    const conduit_cpp::Node params = conduit_cpp::cpp_node(const_cast<conduit_node *>(cparams));
    if (params.has_child("vistle")) {
        parseParams(
            params, ConfigEntry<int>{&connectionParams.paused, "vistle/paused", &conduit_cpp::Node::to_int},
            ConfigEntry<std::string>{&connectionParams.launchOptions, "vistle/options", &conduit_cpp::Node::as_string},
            ConfigEntry<int>{&connectionParams.dynamicGrid, "vistle/dynamic_grid", &conduit_cpp::Node::to_int},
            ConfigEntry<int64_t>{&connectionParams.fortranComm, "catalyst/mpi_comm", &conduit_cpp::Node::to_int64});
        connectionParams.comm = MPI_Comm_f2c(connectionParams.fortranComm);
    } else {
        auto paused = getenv("VISTLE_INSITU_PAUSED");
        if (paused &&
            (strcasecmp(paused, "true") == 0 || strcasecmp(paused, "on") == 0 || strcasecmp(paused, "1") == 0)) {
            connectionParams.paused = true;
        }
        auto options = getenv("VISTLE_INSITU_OPTIONS");
        if (options) {
            connectionParams.launchOptions = options;
        }
        connectionParams.comm = MPI_COMM_WORLD;
    }
}
static std::map<std::string, vistle::Object::ptr> cachedMeshes;
/**
   * Initialize Catalyst. Follow ParaView protocol
   * https://docs.paraview.org/en/latest/Catalyst/blueprints.html#protocol-initialize
   * additional paramets for vistle (under the "vistle" node are:
   *  paused: 1 or 0 -> wait for the user to start the simulation/visualization
   *  options: string -> additional launch options for vistle in singe process mode (not supported yet)
   */
enum catalyst_status catalyst_initialize_vistle(const conduit_node *cparams)
{
    parseConnectionParams(cparams);
    return catalyst_status_ok;
}

vistle::Object::ptr getMesh(const conduit_cpp::Node &mesh, float time, int timestep)
{
    auto vistleMesh = conduitMeshToVistle(mesh, adapter->moduleId());
    vistleMesh->setRealTime(time);
    vistleMesh->setTimestep(timestep);
    adapter->updateMeta(vistleMesh);
    if (!connectionParams.dynamicGrid) {
        cachedMeshes[mesh.name()] = vistleMesh;
    }
    return vistleMesh;
}

void printChildren(const conduit_cpp::Node &node, int level = 0)
{
    if (node.number_of_children() > 20) {
        std::cerr << std::string(level * 2, ' ');
        std::cerr << node.name() << " has too many children (" << node.number_of_children() << "), skipping print"
                  << std::endl;
        return;
    }
    for (int i = 0; i < node.number_of_children(); ++i) {
        auto child = node.child(i);
        std::cerr << std::string(level * 2, ' ');
        std::string addon;
        constexpr size_t maxAddonLength = 40;
        int offset = (int)maxAddonLength - (level * 2 + child.name().size());
        if (offset >= 0)
            addon = std::string(offset, ' ');
        if (child.dtype().is_string())
            addon += child.as_string();
        else if (child.dtype().is_number())
            addon += std::to_string(child.to_float64());
        else
            addon += "num elements : " + std::to_string(child.dtype().number_of_elements());
        std::cerr << child.name() << " " << addon << std::endl;
        printChildren(child, level + 1);
    }
}

void printNodeWithoutArrays(const conduit_cpp::Node &node)
{
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank != 0)
        return;
    printChildren(node);
}


vistle::insitu::MetaData metaDataFromConduit(const conduit_node *cparams)
{
    const conduit_cpp::Node params = conduit_cpp::cpp_node(const_cast<conduit_node *>(cparams));
    printNodeWithoutArrays(params);

    auto catalystNode = params["catalyst"];


    vistle::insitu::MetaData metaData;
    if (!catalystNode.has_child("channels")) {
        std::cerr << "no channels found" << std::endl;
        return metaData;
    }
    auto channels = catalystNode["channels"];
    for (conduit_index_t i = 0; i < channels.number_of_children(); i++) {
        auto mesh = channels.child(i);
        auto meshName = mesh.name();
        auto type = mesh["type"].as_string(); // Currently, the only supported value is mesh (no multimesh).
        if (type != "mesh") {
            std::cerr << mesh.name() << ": unsupported channel type: " << type << std::endl;
            continue;
        }
        if (mesh.has_child("data")) {
            std::cerr << "using 'data' node for mesh " << meshName << std::endl;
            mesh = mesh["data"];
        } // ParaView is using the "data" node to support other mesh types, maybe this is how it should be
        conduit_cpp::Node info;
        bool is_valid = conduit_cpp::Blueprint::verify("mesh", mesh, info);
        if (!is_valid) {
            std::cerr << "'data' on channel '" << meshName << "' is not a valid 'mesh'; skipping channel." << std::endl;
            std::cerr << info.to_yaml() << std::endl;
            continue;
        }
        vistle::insitu::MetaMesh metaMesh(meshName);

        auto fields = mesh["fields"];
        for (conduit_index_t fieldNum = 0; fieldNum < fields.number_of_children(); fieldNum++) {
            auto field = fields.child(fieldNum);
            metaMesh.addVar(field.name());
        }
        metaData.addMesh(metaMesh);
    }
    return metaData;
}

int getTimestep(const conduit_cpp::Node &params)
{
    auto catalystNode = params["catalyst"];
    if (catalystNode.has_child("state/timestep"))
        return catalystNode["state/timestep"].to_int();
    if (params.has_child("time/timestep/cycle"))
        return params["time/timestep/cycle"].to_int();
    return 0;
}

vistle::insitu::ObjectRetriever::PortAssignedObjectList getDataFromSim(const vistle::insitu::MetaData &usedData)
{
    assert(currentCNode);
    const conduit_cpp::Node params = conduit_cpp::cpp_node(const_cast<conduit_node *>(currentCNode));
    int timestep = getTimestep(params);
    auto catalystNode = params["catalyst"];
    float time = 0.0;
    if (catalystNode.has_child("state/time"))
        time = catalystNode["state/time"].to_float64();


    vistle::insitu::ObjectRetriever::PortAssignedObjectList objects;

    for (const auto &requestedMesh: usedData) {
        std::cerr << "fetching requested mesh: " << requestedMesh.name() << std::endl;
        auto mesh = params["catalyst/channels/" + requestedMesh.name()];
        if (mesh.has_child("data")) {
            mesh = mesh["data"];
            std::cerr << "using 'data' node for mesh " << requestedMesh.name() << std::endl;
        } // ParaView is using the "data" node to support other mesh types, maybe this is how it should be

        auto cachedMeshIt = cachedMeshes.find(requestedMesh.name());
        auto vistleMesh = (connectionParams.dynamicGrid || cachedMeshIt == cachedMeshes.end())
                              ? getMesh(mesh, time, timestep)
                              : cachedMeshIt->second;

        objects.push_back(vistle::insitu::ObjectRetriever::PortAssignedObject(requestedMesh.name(), vistleMesh));
        for (const auto &requestedField: requestedMesh) {
            auto field = mesh["fields/" + requestedField];
            auto data = conduitDataToVistle(field, adapter->moduleId());
            if (!data) {
                std::cerr << "failed to convert field " << requestedField << " to vistle" << std::endl;
                continue;
            }
            data->setRealTime(time);
            data->setGrid(vistleMesh);
            adapter->updateMeta(data);
            objects.push_back(
                vistle::insitu::ObjectRetriever::PortAssignedObject(requestedMesh.name(), requestedField, data));
        }
    }
    return objects;
}

/**
   * Execute per timestep.
   * Following the ParaView execute protocol
   * https://docs.paraview.org/en/latest/Catalyst/blueprints.html#protocol-execute
   */
enum catalyst_status catalyst_execute_vistle(const conduit_node *cparams)
{
    currentCNode = cparams;

    //initialize Vistle
    if (!adapter) {
        vistle::insitu::ObjectRetriever getter(&getDataFromSim);
        adapter.reset(new vistle::insitu::Adapter(connectionParams.paused, connectionParams.comm,
                                                  metaDataFromConduit(cparams), getter, VISTLE_ROOT, VISTLE_BUILD_TYPE,
                                                  connectionParams.launchOptions));
    }
    adapter->execute(getTimestep(conduit_cpp::cpp_node(const_cast<conduit_node *>(cparams))));
    return catalyst_status_ok;
}

/**
   * finalize catalyst.
   */
enum catalyst_status catalyst_finalize_vistle(const conduit_node *params)
{
    return catalyst_status_ok;
}

/**
   * Returns information about the catalyst library
   */
enum catalyst_status catalyst_about_vistle(conduit_node *params)
{
    // convert to conduit::Node
    conduit::Node &cpp_params = (*conduit::cpp_node(params));

    // now, use conduit::Node API.
    cpp_params["catalyst"]["capabilities"].append().set("adaptor0");
    return catalyst_status_ok;
}

/**
   * Get results from the catalyst library.
   */
enum catalyst_status catalyst_results_vistle(conduit_node *params)
{
    return catalyst_status_ok;
}
