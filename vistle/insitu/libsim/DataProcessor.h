#ifndef VISTLE_LIBSIM_DATA_PROCESSOR_H
#define VISTLE_LIBSIM_DATA_PROCESSOR_H

#include "MeshInfo.h"
#include "MetaData.h"
#include "MeshInfo.h"
#include "VariableInfo.h"

#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>
#include <vistle/insitu/message/addObjectMsq.h>

#include <vistle/core/vec.h>

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
	DataTransmitter(const MetaData & metaData, message::SyncShmIDs &creator, const ModuleInfo& moduleInfo, int rank);
	void transferObjectsToVistle(size_t timestep, const std::set<std::string> &connectedPorts, const Rules & rules);
private:
    std::set<std::string> getRequestedObjets(const std::set<std::string>& connectedPorts);
    void sendMeshesToModule(const std::set<std::string>& objects);
    MeshInfo collectMeshInfo(size_t nthMesh);
    bool sendConstantMesh(const MeshInfo& meshInfo);

    void makeRectilinearMeshes(MeshInfo& meshInfo);
    void makeRectilinearMesh(int currDomain, MeshInfo& meshInfo);

    void makeUnstructuredMeshes(MeshInfo& meshInfo);
    void makeUnstructuredMesh(int currDomain, MeshInfo& meshInfo);

    void makeStructuredMeshes(MeshInfo& meshInfo);
    void makeStructuredMesh(int currDomain, MeshInfo& meshInfo);

    void makeAmrMesh(MeshInfo& meshInfo);

    //combine the structured meshes of one domain to a singe unstructured mesh. Points of adjacent faces will be doubled.
    void combineStructuredMeshesToUnstructured(MeshInfo& meshInfo);
    void combineRectilinearToUnstructured(MeshInfo& meshInfo);

    void addBlockToMeshInfo(vistle::Object::ptr grid, MeshInfo& meshInfo, visit_handle meshHandle = visit_handle{});
    void sendMeshToModule(const MeshInfo& meshInfo);

    void sendVarablesToModule(const std::set<std::string>& objects);
    VariableInfo collectVariableInfo(size_t nthVariable);
    vistle::Object::ptr makeCombinedVariable(const VariableInfo& varInfo);
    vistle::Object::ptr makeVtkVariable(const VariableInfo& varInfo, int iteration);
    vistle::Object::ptr makeVariable(const VariableInfo& varInfo, int iteration);
    void sendVarableToModule(vistle::Object::ptr variable, int block, const char* name);

    void setTimestep(vistle::Object::ptr data);
    void setTimestep(vistle::Vec<vistle::Scalar, 1>::ptr vec);
    bool isRequested(const char* objectName, const std::set<std::string>& objects);

    int m_rank = 0;
    size_t m_currTimestep = 0;
	message::SyncShmIDs& m_creator;
    message::AddObjectMsq m_sender;
    const MetaData& m_metaData;
    Rules m_rules;
    std::map<std::string, MeshInfo> m_meshes; //used to find the coresponding mesh for the variables

};

}
}
}


#endif // !VISTLE_LIBSIM_DATA_PROCESSOR_H
