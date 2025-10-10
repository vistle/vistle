#include "DataTransmitter.h"
#include "ArrayStruct.h"
#include "Exception.h"
#include "RectilinearMesh.h"
#include "StructuredMesh.h"
#include "UnstructuredMesh.h"
#include "VariableData.h"
#include "VisitDataTypesToVistle.h"

#include <vistle/insitu/core/transformArray.h>

#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/insitu/message/addObjectMsq.h>

#include <vistle/insitu/libsim/libsimInterface/DomainList.h>
#include <vistle/insitu/libsim/libsimInterface/MeshMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/VariableData.h>
#include <vistle/insitu/libsim/libsimInterface/VariableMetaData.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataInterfaceRuntime.h>

using namespace vistle::insitu::libsim;
namespace vistle {
namespace insitu {
namespace libsim {
struct DataTransmitterException: public vistle::insitu::InsituException {
    DataTransmitterException() { *this << "DataTransmitter: "; }
};
} // namespace libsim
} // namespace insitu
} // namespace vistle

DataTransmitter::DataTransmitter(const MetaData &metaData, const message::ModuleInfo &moduleInfo, int rank)
: m_metaData(metaData), m_sender(moduleInfo, rank), m_rank(rank)
{}

void DataTransmitter::transferObjectsToVistle(size_t timestep, size_t iteration, const message::ModuleInfo &moduleState,
                                              const Rules &rules)
{
    m_currTimestep = timestep;
    m_currIteration = iteration;
    m_rules = rules;
    sendMeshesToModule(m_requestedObjects, moduleState);
    sendVarablesToModule(m_requestedObjects, moduleState);
    m_sender.sendObjects();
}

void DataTransmitter::resetCache()
{
    m_meshes.clear();
}

void DataTransmitter::updateRequestedObjets(const message::ModuleInfo &connectedPorts)
{
    m_requestedObjects.clear();
    size_t numVars = m_metaData.getNumObjects(SimulationObjectType::variable);
    auto meshes = m_metaData.getObjectNames(SimulationObjectType::mesh);
    for (const auto &mesh: meshes) {
        if (connectedPorts.isPortConnected(mesh)) {
            m_requestedObjects.insert(mesh);
        }
    }
    for (size_t i = 0; i < numVars; i++) {
        char *name, *meshName;
        visit_handle varMetaHandle = m_metaData.getNthObject(SimulationObjectType::variable, i);
        v2check(simv2_VariableMetaData_getName, varMetaHandle, &name);
        v2check(simv2_VariableMetaData_getMeshName, varMetaHandle, &meshName);
        if (connectedPorts.isPortConnected(name)) {
            m_requestedObjects.insert(name);
            m_requestedObjects.insert(meshName);
        }
        free(name);
        free(meshName);
    }
}

void DataTransmitter::updateGeneration()
{
    ++m_generation;
}


// private
void DataTransmitter::sendMeshesToModule(const std::set<std::string> &objects, const message::ModuleInfo &moduleInfo)
{
    size_t numMeshes = m_metaData.getNumObjects(SimulationObjectType::mesh);
    for (size_t i = 0; i < numMeshes; i++) {
        MeshInfo meshInfo = collectMeshInfo(i);
        if (isRequested(meshInfo.name.c_str(), objects) && !constantMeshCreated(meshInfo)) {
            makeMesh(meshInfo);
            sendMeshToModule(meshInfo, moduleInfo);
        }
    }
}

MeshInfo DataTransmitter::collectMeshInfo(size_t nthMesh)
{
    auto type = SimulationObjectType::mesh;
    MeshInfo meshInfo;
    visit_handle meshHandle = m_metaData.getNthObject(type, nthMesh);
    meshInfo.name = m_metaData.getName(meshHandle, type);
    meshInfo.domainHandle = v2check(simv2_invoke_GetDomainList, meshInfo.name.c_str());
    int allDoms = 0;
    visit_handle myDoms;
    v2check(simv2_DomainList_getData, meshInfo.domainHandle.operator visit_handle(), allDoms, myDoms);

    auto domList = getVariableData(myDoms);
    if (domList.type != VISIT_DATATYPE_INT) {
        throw DataTransmitterException{} << "expected domain list to be ints";
    }
    meshInfo.domains = domList;

    v2check(simv2_MeshMetaData_getMeshType, meshHandle, (int *)&meshInfo.type);
    v2check(simv2_MeshMetaData_getTopologicalDimension, meshHandle, &meshInfo.dim);
    return meshInfo;
}

bool DataTransmitter::constantMeshCreated(const MeshInfo &meshInfo)
{
    return (m_rules.constGrids && m_meshes.find(meshInfo.name) != m_meshes.end());
}

void DataTransmitter::makeMesh(MeshInfo &meshInfo)
{
    if (m_rules.combineGrid) {
        makeCombinedMesh(meshInfo);
    } else {
        makeSeparateMeshes(meshInfo);
    }
}

void DataTransmitter::makeSeparateMeshes(MeshInfo &meshInfo)
{
    for (const auto dom: meshInfo.domains.getIter<int>()) {
        makeSubMesh(dom, meshInfo);
    }
    m_meshes[meshInfo.name] = meshInfo;
}

void DataTransmitter::makeCombinedMesh(MeshInfo &meshInfo)
{
    vistle::Object::ptr mesh;
    switch (meshInfo.type) {
    case VISIT_MESHTYPE_AMR:
    case VISIT_MESHTYPE_RECTILINEAR: {
        mesh = RectilinearMesh::getCombinedUnstructured(meshInfo, m_rules.vtkFormat);
        break;
    case VISIT_MESHTYPE_CURVILINEAR: {
        mesh = StructuredMesh::getCombinedUnstructured(meshInfo, m_rules.vtkFormat);
    } break;
    case VISIT_MESHTYPE_UNSTRUCTURED: {
        makeMesh(meshInfo);
        return;
    } break;
    default:
        throw EngineException("meshtype ") << static_cast<int>(meshInfo.type) << " not implemented";
        break;
    }
    }
    meshInfo.combined = true;
    meshInfo.grids.push_back(mesh);
    m_meshes[meshInfo.name] = meshInfo;
}

void DataTransmitter::makeSubMesh(int domain, MeshInfo &meshInfo)
{
    vistle::Object::ptr mesh;
    switch (meshInfo.type) {
    case VISIT_MESHTYPE_AMR: /*  */
    case VISIT_MESHTYPE_RECTILINEAR: {
        visit_smart_handle<HandleType::RectilinearMesh> meshHandle =
            v2check(simv2_invoke_GetMesh, domain, meshInfo.name.c_str());
        mesh = get(meshHandle);
    } break;
    case VISIT_MESHTYPE_CURVILINEAR: {
        visit_smart_handle<HandleType::CurvilinearMesh> meshHandle =
            v2check(simv2_invoke_GetMesh, domain, meshInfo.name.c_str());
        mesh = get(meshHandle);
    } break;
    case VISIT_MESHTYPE_UNSTRUCTURED: {
        visit_smart_handle<HandleType::UnstructuredMesh> meshHandle =
            v2check(simv2_invoke_GetMesh, domain, meshInfo.name.c_str());
        mesh = get(meshHandle);
    } break;
    default:
        break;
    }
    if (!mesh)
        throw DataTransmitterException{} << "makeSubMesh failed to get mesh " << meshInfo.name << " dom " << domain;
    meshInfo.grids.push_back(mesh);
}

void DataTransmitter::sendMeshToModule(const MeshInfo &meshInfo, const message::ModuleInfo &moduleInfo)
{
    size_t numMeshes = meshInfo.combined ? 1 : meshInfo.domains.size;
    assert(numMeshes == meshInfo.grids.size());
    for (size_t i = 0; i < numMeshes; i++) {
        int block = meshInfo.combined ? m_rank : meshInfo.domains.as<int>()[i];
        meshInfo.grids[i]->setBlock(block);
        updateMeta(meshInfo.grids[i], moduleInfo);
        m_sender.addObject(meshInfo.name, meshInfo.grids[i]);
    }
}

void DataTransmitter::sendVarablesToModule(const std::set<std::string> &objects, const message::ModuleInfo &moduleInfo)
{
    size_t numVars = m_metaData.getNumObjects(SimulationObjectType::variable);
    for (size_t i = 0; i < numVars; i++) {
        VariableInfo varInfo = collectVariableInfo(i);

        if (isRequested(varInfo.name.c_str(), objects)) {
            if (varInfo.meshInfo.combined) {
                auto var = makeCombinedVariable(varInfo);
                updateMeta(var, moduleInfo);
                sendVarableToModule(var, m_rank, varInfo.name.c_str());

            } else {
                for (size_t iteration = 0; iteration < varInfo.meshInfo.domains.size; ++iteration) {
                    int currDomain = varInfo.meshInfo.domains.as<int>()[iteration];
                    if (auto variable = makeVariable(varInfo, iteration, m_rules.vtkFormat)) {
                        updateMeta(variable, moduleInfo);
                        sendVarableToModule(variable, currDomain, varInfo.name.c_str());
                    } else {
                        std::cerr << "sendVarablesToModule failed to convert variable " << varInfo.name
                                  << "... trying next" << std::endl;
                    }
                }
            }
        }
    }
}

VariableInfo DataTransmitter::collectVariableInfo(size_t nthVariable)
{
    visit_handle varMetaHandle = m_metaData.getNthObject(SimulationObjectType::variable, nthVariable);
    char *meshName;
    v2check(simv2_VariableMetaData_getMeshName, varMetaHandle, &meshName);

    auto meshInfo = m_meshes.find(meshName);
    free(meshName);
    if (meshInfo == m_meshes.end()) {
        throw DataTransmitterException{} << "sendVarablesToModule: can't find mesh " << meshName;
    }
    VariableInfo varInfo(meshInfo->second);
    char *name;
    v2check(simv2_VariableMetaData_getName, varMetaHandle, &name);
    varInfo.name = name;
    free(name);
    int centering;
    v2check(simv2_VariableMetaData_getCentering, varMetaHandle, &centering);
    varInfo.mapping = mappingToVistle(centering);
    return varInfo;
}

vistle::DataBase::ptr DataTransmitter::makeCombinedVariable(const VariableInfo &varInfo)
{
    if (m_rules.vtkFormat) {
        std::cerr << "combined grids in vtk format are not supported! Adding field data may fail." << std::endl;
    }
    vistle::Vec<vistle::Scalar, 1>::ptr variable;
    variable = VariableData::allocVarForCombinedMesh(varInfo, varInfo.meshInfo.grids[0]);
    size_t combinedVecPos = 0;
    for (const auto dom: varInfo.meshInfo.domains.getIter<int>()) {
        visit_smart_handle<HandleType::VariableData> varHandle =
            v2check(simv2_invoke_GetVariable, dom, varInfo.name.c_str());
        auto varArray = getVariableData(varHandle);

        assert(combinedVecPos + varArray.size <= variable->x().size());
        transformArray(varArray, variable->x().data() + combinedVecPos);
        combinedVecPos += varArray.size;
    }
    return variable;
}

vistle::DataBase::ptr DataTransmitter::makeVariable(const VariableInfo &varInfo, int iteration, bool vtkFormat)
{
    int currDomain = varInfo.meshInfo.domains.as<int>()[iteration];
    visit_smart_handle<HandleType::VariableData> varHandle =
        v2check(simv2_invoke_GetVariable, currDomain, varInfo.name.c_str());
    auto varArray = getVariableData(varHandle);
    if (vtkFormat) {
        auto var = vtkData2Vistle(varArray.data, varArray.size, dataTypeToVistle(varArray.type),
                                  varInfo.meshInfo.grids[iteration], varInfo.mapping);
        return var;
    } else {
        auto var = std::make_shared<vistle::Vec<vistle::Scalar, 1>>((Index)varArray.size);
        transformArray(varArray, var->x().data());
        var->setMapping(varInfo.mapping);
        var->setGrid(varInfo.meshInfo.grids[iteration]);
        return var;
    }
}

void DataTransmitter::sendVarableToModule(vistle::DataBase::ptr variable, int block, const char *name)
{
    variable->setBlock(block);
    variable->describe(name, m_sender.moduleId());
    m_sender.addObject(name, variable);
}

void DataTransmitter::setMeshTimestep(vistle::Object::ptr mesh)
{
    int step = m_currTimestep;
    if (m_rules.constGrids) {
        step = -1;
    }
    setTimestep(mesh, step);
}

void DataTransmitter::setTimestep(vistle::Object::ptr variable, size_t timestep)
{
    if (m_rules.keepTimesteps) {
        variable->setTimestep(timestep);
    } else {
        variable->setIteration(timestep);
    }
}

bool DataTransmitter::isRequested(const char *objectName, const std::set<std::string> &requestedObjects)
{
    return requestedObjects.find(objectName) != requestedObjects.end();
}


void DataTransmitter::updateMeta(vistle::Object::ptr obj, const message::ModuleInfo &moduleInfo) const
{
    if (obj) {
        obj->setCreator(moduleInfo.id());
        obj->setGeneration(m_generation);
        obj->setTimestep(m_currTimestep);
        obj->setIteration(m_currIteration);
        obj->updateInternals();
    }
}
