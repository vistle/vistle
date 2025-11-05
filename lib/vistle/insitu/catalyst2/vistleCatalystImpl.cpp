

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

static std::unique_ptr<vistle::insitu::Adapter> adapter = nullptr;

struct ConnectionParams {
    int paused = false;
    int dynamicGrid = false;
    std::string launchOptions;
    MPI_Comm comm;
    int64_t fortranComm = 0;
};

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
    assert(params.has_child("vistle"));
    parseParams(
        params, ConfigEntry<int>{&connectionParams.paused, "vistle/paused", &conduit_cpp::Node::to_int},
        ConfigEntry<std::string>{&connectionParams.launchOptions, "vistle/options", &conduit_cpp::Node::as_string},
        ConfigEntry<int>{&connectionParams.dynamicGrid, "vistle/dynamic_grid", &conduit_cpp::Node::to_int},
        ConfigEntry<int64_t>{&connectionParams.fortranComm, "catalyst/mpi_comm", &conduit_cpp::Node::to_int64});
    connectionParams.comm = MPI_Comm_f2c(connectionParams.fortranComm);
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

/**
   * Execute per timestep.
   * Following the ParaView execute protocol
   * https://docs.paraview.org/en/latest/Catalyst/blueprints.html#protocol-execute
   */
enum catalyst_status catalyst_execute_vistle(const conduit_node *cparams)
{
    const conduit_cpp::Node params = conduit_cpp::cpp_node(const_cast<conduit_node *>(cparams));
    auto cycle =
        params["time/timestep/cycle"].to_int(); // this defines temporal information about the current invocation.
    auto catalystNode = params["catalyst"];
    int timestep = cycle;
    if (catalystNode.has_child("state/timestep"))
        timestep =
            catalystNode["state/timestep"]
                .to_int(); // (optional) integral value for current timestep. if not specified, catalyst/cycle is used.

    float time = 0.0;
    if (catalystNode.has_child("state/time"))
        time = catalystNode["state/time"]
                   .to_float64(); // (optional) float64 value for current time, if not specified, 0.0 is assumed.
    // auto parameters = params["catalyst/state/parameters"].as_string(); // (optional) list of optional runtime parameters. If present, they must be of type list with each child node of type string.
    // auto multiblock = params["catalyst/state/multiblock"].as_int(); // (optional) integral value. When present and set to 1, output will be a legacy vtkMultiBlockDataSet.

    vistle::insitu::MetaData metaData;
    if (!catalystNode.has_child("channels")) {
        std::cerr << "no channels found" << std::endl;
        return catalyst_status_ok;
    }
    auto channels = catalystNode["channels"];
    for (conduit_index_t i = 0; i < channels.number_of_children(); i++) {
        auto mesh = channels.child(i);
        auto type = mesh["type"].as_string(); // Currently, the only supported value is mesh (no multimesh).
        if (type != "mesh") {
            std::cerr << mesh.name() << ": unsupported channel type: " << type << std::endl;
            continue;
        }
        conduit_cpp::Node info;
        bool is_valid = conduit_cpp::Blueprint::verify("mesh", mesh, info);
        if (!is_valid) {
            std::cerr << "'data' on channel '" << mesh.name() << "' is not a valid 'mesh'; skipping channel."
                      << std::endl;
            continue;
        }
        vistle::insitu::MetaMesh metaMesh(mesh.name());

        auto fields = mesh["fields"];
        for (conduit_index_t fieldNum = 0; fieldNum < fields.number_of_children(); fieldNum++) {
            auto field = fields.child(fieldNum);
            metaMesh.addVar(field.name());
        }
        metaData.addMesh(metaMesh);
    }


    vistle::insitu::ObjectRetriever cbs{[&](const vistle::insitu::MetaData &usedData) {
        vistle::insitu::ObjectRetriever::PortAssignedObjectList objects;
        for (const auto &requestedMesh: usedData) {
            auto mesh = params["catalyst/channels/" + requestedMesh.name()];
            auto vistleMesh =
                (connectionParams.dynamicGrid || cachedMeshes.find(requestedMesh.name()) == cachedMeshes.end())
                    ? getMesh(mesh, time, timestep)
                    : cachedMeshes[requestedMesh.name()];

            objects.push_back(vistle::insitu::ObjectRetriever::PortAssignedObject(requestedMesh.name(), vistleMesh));
            for (const auto &requestedField: requestedMesh) {
                auto field = mesh["fields/" + requestedField];
                auto data = conduitDataToVistle(field, adapter->moduleId());
                data->setRealTime(time);
                data->setGrid(vistleMesh);
                data->setTimestep(timestep);
                adapter->updateMeta(data);
                objects.push_back(
                    vistle::insitu::ObjectRetriever::PortAssignedObject(requestedMesh.name(), requestedField, data));
            }
        }
        return objects;
    }};

    //initialize Vistle
    if (!adapter) {
        adapter.reset(new vistle::insitu::Adapter(connectionParams.paused, connectionParams.comm, std::move(metaData),
                                                  cbs, VISTLE_ROOT, VISTLE_BUILD_TYPE, connectionParams.launchOptions));
    }
    adapter->execute(timestep);
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
