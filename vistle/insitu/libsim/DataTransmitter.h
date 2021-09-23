#ifndef VISTLE_LIBSIM_DATA_TRANSMITTER_H
#define VISTLE_LIBSIM_DATA_TRANSMITTER_H

#include "MeshInfo.h"
#include "MetaData.h"
#include "VariableInfo.h"

#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>
#include <vistle/insitu/message/addObjectMsq.h>

#include <vistle/core/vec.h>

#include <functional>
namespace vistle {
namespace insitu {
namespace message {
class SyncShmIDs;
}
namespace libsim {
struct Rules {
    bool combineGrid = false;
    bool vtkFormat = false;
    bool constGrids = false;
    bool keepTimesteps = false;
};
class DataTransmitter {
public:
    DataTransmitter(const MetaData &metaData, message::SyncShmIDs &creator, const message::ModuleInfo &moduleInfo,
                    int rank);
    void transferObjectsToVistle(size_t timestep, const message::ModuleInfo &connectedPorts, const Rules &rules);
    void resetCache();

private:
    typedef std::function<vistle::Object::ptr(const visit_handle &, vistle::insitu::message::SyncShmIDs &)>
        GetMeshFunction;

    std::set<std::string> getRequestedObjets(const message::ModuleInfo &connectedPorts);
    void sendMeshesToModule(const std::set<std::string> &objects);
    MeshInfo collectMeshInfo(size_t nthMesh);
    bool sendConstantMesh(const MeshInfo &meshInfo);

    void makeMesh(MeshInfo &meshInfo);
    void makeSeparateMeshes(MeshInfo &meshInfo);
    GetMeshFunction chooseMeshMaker(VisIt_MeshType type);
    // combine the rectilinear/structured meshes of the domains of this rank to a single unstructured grid. Points of
    // adjacent faces will be doubled.
    void makeCombinedMesh(MeshInfo &meshInfo);
    void makeSubMesh(int domain, MeshInfo &meshInfo);

    void sendMeshToModule(const MeshInfo &meshInfo);

    void sendVarablesToModule(const std::set<std::string> &objects);
    VariableInfo collectVariableInfo(size_t nthVariable);
    vistle::Object::ptr makeCombinedVariable(const VariableInfo &varInfo);
    vistle::Object::ptr makeVariable(const VariableInfo &varInfo, int iteration, bool vtkFormat);
    void sendVarableToModule(vistle::Object::ptr variable, int block, const char *name);

    void setMeshTimestep(vistle::Object::ptr mesh);
    void setTimestep(vistle::Object::ptr variable, size_t timestep);
    bool isRequested(const char *objectName, const std::set<std::string> &requestedObjects);

    const MetaData &m_metaData;
    message::SyncShmIDs &m_creator;
    message::AddObjectMsq m_sender;
    int m_rank = 0;

    size_t m_currTimestep = 0;
    Rules m_rules;
    std::map<std::string, MeshInfo> m_meshes; // used to find the corresponding mesh for the variables
};

} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif // !VISTLE_LIBSIM_DATA_PROCESSOR_H
