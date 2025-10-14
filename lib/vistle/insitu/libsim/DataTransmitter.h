#ifndef VISTLE_INSITU_LIBSIM_DATATRANSMITTER_H
#define VISTLE_INSITU_LIBSIM_DATATRANSMITTER_H

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
    bool keepTimesteps = true;
    int frequency = 1;
};
class DataTransmitter {
public:
    DataTransmitter(const MetaData &metaData, const message::ModuleInfo &moduleInfo, int rank);
    void transferObjectsToVistle(size_t timestep, size_t iteration, const message::ModuleInfo &moduleState,
                                 const Rules &rules);
    void resetCache();

    void updateRequestedObjets(const message::ModuleInfo &connectedPorts);
    void updateGeneration();

private:
    typedef std::function<vistle::Object::ptr(const visit_handle &, vistle::insitu::message::SyncShmIDs &)>
        GetMeshFunction;

    void sendMeshesToModule(const std::set<std::string> &objects, const message::ModuleInfo &moduleInfo);
    MeshInfo collectMeshInfo(size_t nthMesh);
    bool constantMeshCreated(const MeshInfo &meshInfo);

    void makeMesh(MeshInfo &meshInfo);
    void makeSeparateMeshes(MeshInfo &meshInfo);
    GetMeshFunction chooseMeshMaker(VisIt_MeshType type);
    // combine the rectilinear/structured meshes of the domains of this rank to a single unstructured grid. Points of
    // adjacent faces will be doubled.
    void makeCombinedMesh(MeshInfo &meshInfo);
    void makeSubMesh(int domain, MeshInfo &meshInfo);

    void sendMeshToModule(const MeshInfo &meshInfo, const message::ModuleInfo &moduleInfo);

    void sendVarablesToModule(const std::set<std::string> &objects, const message::ModuleInfo &moduleInfo);
    VariableInfo collectVariableInfo(size_t nthVariable);
    vistle::DataBase::ptr makeCombinedVariable(const VariableInfo &varInfo);
    vistle::DataBase::ptr makeVariable(const VariableInfo &varInfo, int iteration, bool vtkFormat);
    void sendVarableToModule(vistle::DataBase::ptr variable, int block, const char *name);

    void setMeshTimestep(vistle::Object::ptr mesh);
    void setTimestep(vistle::Object::ptr variable, size_t timestep);
    bool isRequested(const char *objectName, const std::set<std::string> &requestedObjects);
    void updateMeta(vistle::Object::ptr obj, const message::ModuleInfo &moduleInfo) const;

    const MetaData &m_metaData;
    message::AddObjectMsq m_sender;
    int m_rank = 0;

    size_t m_currTimestep = 0;
    size_t m_currIteration = 0;
    size_t m_generation = 0;
    Rules m_rules;
    //the data objects that need to be procces according to the modules conneted ports
    std::set<std::string> m_requestedObjects;
    std::map<std::string, MeshInfo> m_meshes; // used to find the corresponding mesh for the variables
};

} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif
