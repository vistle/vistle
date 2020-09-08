#include "DataProcessor.h"
#include "Exeption.h"
#include "RectilinearMesh.h"
#include "StructuredMesh.h"
#include "UnstructuredMesh.h"
#include "VariableData.h"
#include "VisitDataTypesToVistle.h"
#include "ArrayStruct.h"

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
struct DataTransmitterExeption : public vistle::insitu::InsituExeption
{
    DataTransmitterExeption() 
    {
        *this << "DataTransmitter: ";
    }
};
}
}
}


DataTransmitter::DataTransmitter(const MetaData &metaData, message::SyncShmIDs &creator, const ModuleInfo& moduleInfo, int rank)
    :m_creator(creator)
    ,m_sender(moduleInfo, rank)
    ,m_metaData(metaData)
    ,m_rank(rank)
{

}

void DataTransmitter::transferObjectsToVistle(size_t timestep, const std::set<std::string>& connectedPorts, const Rules& rules)
{
    m_currTimestep = timestep;
    m_rules = rules;
    auto requestedObjects = getRequestedObjets(connectedPorts);

    sendMeshesToModule(requestedObjects);
    sendVarablesToModule(requestedObjects);

}

std::set<std::string> DataTransmitter::getRequestedObjets(const std::set<std::string>& connectedPorts) {

    std::set<std::string> requested;
    int numVars = m_metaData.getNumObjects(SimulationObjectType::variable);
    auto meshes = m_metaData.getObjectNames(SimulationObjectType::mesh);
    for (const auto mesh : meshes)
    {
        if (connectedPorts.find(mesh) != connectedPorts.end())
        {
            requested.insert(mesh);
        }
    }
    for (size_t i = 0; i < numVars; i++)
    {
        char* name, *meshName;
        visit_handle varMetaHandle = m_metaData.getNthObject(SimulationObjectType::variable, i);
        v2check(simv2_VariableMetaData_getName, varMetaHandle, &name);
        v2check(simv2_VariableMetaData_getMeshName, varMetaHandle, &meshName);
        if (connectedPorts.find(name) != connectedPorts.end())
        {
            requested.insert(name);
            requested.insert(meshName);
        }
    }
    return requested;
}

void DataTransmitter::sendMeshesToModule(const std::set<std::string>& objects) {
    auto type = SimulationObjectType::mesh;
    int numMeshes = m_metaData.getNumObjects(type);
    for (size_t i = 0; i < numMeshes; i++) {
        MeshInfo meshInfo = collectMeshInfo(i);
        if (!isRequested(meshInfo.name, objects) || sendConstantMesh(meshInfo))
        {
            continue;
        }

        switch (meshInfo.type) {
        case VISIT_MESHTYPE_RECTILINEAR:
        {
            if (m_rules.combineGrid) {
                combineRectilinearToUnstructured(meshInfo);
            }
            else {
                makeRectilinearMeshes(meshInfo);
            }
        }
        break;
        case VISIT_MESHTYPE_CURVILINEAR:
        {
            if (m_rules.combineGrid) {
                //to do:read domain boundaries if set
                combineStructuredMeshesToUnstructured(meshInfo);
            }
            else {
                makeStructuredMeshes(meshInfo);
            }
        }
        break;
        case VISIT_MESHTYPE_UNSTRUCTURED:
        {
            makeUnstructuredMeshes(meshInfo);
        }
        break;
        case VISIT_MESHTYPE_AMR:
        {
            makeAmrMesh(meshInfo);
        }
        break;
        default:
            throw EngineExeption("meshtype ") << static_cast<int>(meshInfo.type) << " not implemented";
            break;
        }
        sendMeshToModule(meshInfo);

    }
}

MeshInfo DataTransmitter::collectMeshInfo(size_t nthMesh)
{
    auto type = SimulationObjectType::mesh;
    MeshInfo meshInfo;
    visit_handle meshHandle = m_metaData.getNthObject(type, nthMesh);
    meshInfo.name = m_metaData.getName(meshHandle, type);
    visit_handle domainListHandle = v2check(simv2_invoke_GetDomainList, meshInfo.name);
    int allDoms = 0;
    visit_handle myDoms;
    v2check(simv2_DomainList_getData, domainListHandle, allDoms, myDoms);

    Array domList = getVariableData(myDoms); 
    if (domList.type != VISIT_DATATYPE_INT) {
        throw DataTransmitterExeption{} << "expected domain list to be ints";
    }
    meshInfo.domains = static_cast<const int*>(domList.data);

    v2check(simv2_MeshMetaData_getMeshType, meshHandle, (int*)&meshInfo.type);
    v2check(simv2_MeshMetaData_getTopologicalDimension, meshHandle, &meshInfo.dim);
    return meshInfo;
}

bool DataTransmitter::sendConstantMesh(const MeshInfo& meshInfo) {
    auto meshIt = m_meshes.find(meshInfo.name);
    if (m_rules.constGrids && meshIt != m_meshes.end()) {
        for (auto grid : meshIt->second.grids) {
            m_sender.addObject(meshInfo.name, grid);
        }
        return true; 
    }
    return false;
}

void DataTransmitter::makeRectilinearMeshes(MeshInfo& meshInfo) {
    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {

        makeRectilinearMesh(meshInfo.domains[cd], meshInfo);
    }
    m_meshes[meshInfo.name] = meshInfo;
}

void DataTransmitter::makeRectilinearMesh(int currDomain, MeshInfo& meshInfo)
{
    visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);

    auto mesh = RectilinearMesh::get(meshHandle, m_creator);
    if (!mesh)
    {
        return;
    }
    addBlockToMeshInfo(mesh, meshInfo, meshHandle);
}

void DataTransmitter::makeUnstructuredMeshes(MeshInfo& meshInfo) {

    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {
        makeUnstructuredMesh(meshInfo.domains[cd], meshInfo);
    }
    m_meshes[meshInfo.name] = meshInfo;
}

void DataTransmitter::makeUnstructuredMesh(int currDomain, MeshInfo& meshInfo)
{
    visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);
    auto mesh = UnstructuredMesh::get(meshHandle, m_creator);
    if (!mesh)
    {
        return;
    }
    addBlockToMeshInfo(mesh, meshInfo, meshHandle);
}

void DataTransmitter::makeStructuredMeshes(MeshInfo& meshInfo) {
    for (size_t cd = 0; cd < meshInfo.numDomains; cd++) {
        makeStructuredMesh(meshInfo.domains[cd], meshInfo);
    }
    m_meshes[meshInfo.name] = meshInfo;
}

void DataTransmitter::makeStructuredMesh(int currDomain, MeshInfo& meshInfo)
{
    visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, meshInfo.name);
    auto mesh = StructuredMesh::get(meshHandle, m_creator);
    if (!mesh)
    {
        return;
    }
    addBlockToMeshInfo(mesh, meshInfo, meshHandle);
}

void DataTransmitter::makeAmrMesh(MeshInfo& meshInfo) {
    if (m_rules.combineGrid) {
        combineRectilinearToUnstructured(meshInfo);
    }
    else {
        makeRectilinearMeshes(meshInfo);
    }
}

void DataTransmitter::combineStructuredMeshesToUnstructured(MeshInfo& meshInfo) {

    auto mesh = StructuredMesh::getCombinedUnstructured(meshInfo, m_creator, m_rules.vtkFormat);
    meshInfo.combined = true;
    addBlockToMeshInfo(mesh, meshInfo);
    m_meshes[meshInfo.name] = meshInfo;

}

void DataTransmitter::combineRectilinearToUnstructured(MeshInfo& meshInfo) {
    auto mesh = RectilinearMesh::getCombinedUnstructured(meshInfo, m_creator, m_rules.vtkFormat);
    meshInfo.combined = true;
    m_meshes[meshInfo.name] = meshInfo;
    addBlockToMeshInfo(mesh, meshInfo);

}

void DataTransmitter::addBlockToMeshInfo(vistle::Object::ptr grid, MeshInfo& meshInfo, visit_handle meshHandle)
{
    assert(meshHandle != visit_handle{} || meshInfo.combined);
    meshInfo.handles.push_back(meshHandle);
    meshInfo.grids.push_back(grid);
}

void DataTransmitter::sendMeshToModule(const MeshInfo& meshInfo)
{
    assert(meshInfo.numDomains == meshInfo.grids.size() || meshInfo.combined);
    for (size_t i = 0; i < meshInfo.numDomains; i++)
    {
        int block = meshInfo.combined ? m_rank : meshInfo.domains[i];
        meshInfo.grids[i]->setBlock(block);
        setTimestep(meshInfo.grids[i]);
        m_sender.addObject(meshInfo.name, meshInfo.grids[i]);

    }
}

void DataTransmitter::sendVarablesToModule(const std::set<std::string>& objects) { 
    int numVars = m_metaData.getNumObjects(SimulationObjectType::variable);
    for (size_t i = 0; i < numVars; i++) {
        VariableInfo varInfo = collectVariableInfo(i);

        if (!isRequested(varInfo.name, objects)) {
            continue;
        }
        if (varInfo.meshInfo.combined) {
            auto var = makeCombinedVariable(varInfo);
            sendVarableToModule(var, m_rank, varInfo.name);

        }
        else
        {
            for (size_t cd = 0; cd < varInfo.meshInfo.numDomains; ++cd) {
                int currDomain = varInfo.meshInfo.domains[cd];


                visit_handle varHandle = v2check(simv2_invoke_GetVariable, currDomain, varInfo.name);
                Array varArray = getVariableData(varHandle);
                vistle::Object::ptr variable;
                if (m_rules.vtkFormat) {
                    variable = makeVtkVariable(varInfo, cd);
                }
                else {
                    variable = makeVariable(varInfo, cd);
                }
                if (variable) {
                    sendVarableToModule(variable, currDomain, varInfo.name);
                }
                else
                {
                    std::cerr << "sendVarablesToModule failed to convert variable " << varInfo.name << "... trying next" << std::endl;
                }
            }
        }
    }
}

VariableInfo DataTransmitter::collectVariableInfo(size_t nthVariable)
{
    visit_handle varMetaHandle = m_metaData.getNthObject(SimulationObjectType::variable, nthVariable);
    char* meshName;
    v2check(simv2_VariableMetaData_getMeshName, varMetaHandle, &meshName);

    auto meshInfo = m_meshes.find(meshName);
    if (meshInfo == m_meshes.end()) {
        EngineExeption ex{ "" };
        ex << "sendVarablesToModule: can't find mesh " << meshName;
        throw ex;
    }
    VariableInfo varInfo(meshInfo->second);
    char* name;
    v2check(simv2_VariableMetaData_getName, varMetaHandle, &name);
    varInfo.name = name;
    int centering;
    v2check(simv2_VariableMetaData_getCentering, varMetaHandle, &centering);
    varInfo.mapping = mappingToVistle(centering);
    return varInfo;
}

vistle::Object::ptr DataTransmitter::makeCombinedVariable(const VariableInfo& varInfo) {
    if (m_rules.vtkFormat)
    {
        std::cerr << "combined grids in vtk format are not supported! Adding field data may fail." << std::endl;
    }
    vistle::Vec<vistle::Scalar, 1>::ptr variable;
    variable = VariableData::allocVarForCombinedMesh(varInfo, varInfo.meshInfo.grids[0], m_creator);
    size_t combinedVecPos = 0;
    for (size_t cd = 0; cd < varInfo.meshInfo.numDomains; ++cd) {
        int currDomain = varInfo.meshInfo.domains[cd];

        visit_handle varHandle = v2check(simv2_invoke_GetVariable, currDomain, varInfo.name);
        Array varArray = getVariableData(varHandle);

        assert(combinedVecPos + varArray.size >= variable->x().size());
        transformArray(varArray, variable->x().data() + combinedVecPos);
        combinedVecPos += varArray.size;
    }
    return variable;
}

vistle::Object::ptr DataTransmitter::makeVtkVariable(const VariableInfo& varInfo, int iteration) {
    int currDomain = varInfo.meshInfo.domains[iteration];

    visit_handle varHandle = v2check(simv2_invoke_GetVariable, currDomain, varInfo.name);
    Array varArray = getVariableData(varHandle);
    auto var = vtkData2Vistle(varArray.data, varArray.size, dataTypeToVistle(varArray.type), varInfo.meshInfo.grids[iteration], varInfo.mapping);
    m_creator.set(vistle::Shm::the().objectID(), vistle::Shm::the().arrayID()); //since vtkData2Vistle creates objects

    return var;
}

vistle::Object::ptr DataTransmitter::makeVariable(const VariableInfo& varInfo, int iteration) {
    int currDomain = varInfo.meshInfo.domains[iteration];
    visit_handle varHandle = v2check(simv2_invoke_GetVariable, currDomain, varInfo.name);
    Array varArray = getVariableData(varHandle);
    auto var = m_creator.createVistleObject<vistle::Vec<vistle::Scalar, 1>>(varArray.size);
    transformArray(varArray, var->x().data());
    var->setGrid(varInfo.meshInfo.grids[iteration]);
    var->setMapping(varInfo.mapping);

    return var;
}

void DataTransmitter::sendVarableToModule(vistle::Object::ptr variable, int block, const char* name) {
    setTimestep(variable);
    variable->setBlock(block);
    variable->addAttribute("_species", name);
    m_sender.addObject(name, variable);

}

void DataTransmitter::setTimestep(vistle::Object::ptr data) {
    int step;
    if (m_rules.constGrids) {
        step = -1;
    }
    else {
        step = m_currTimestep;
    }
    if (m_rules.keepTimesteps) {
        data->setTimestep(step);
    }
    else {
        data->setIteration(step);
    }

}

void DataTransmitter::setTimestep(vistle::Vec<vistle::Scalar, 1>::ptr vec) {
    if (m_rules.keepTimesteps) {
        vec->setTimestep(m_currTimestep);
    }
    else {
        vec->setIteration(m_currTimestep);
    }
}

bool DataTransmitter::isRequested(const char* objectName, const std::set<std::string>& objects)
{
    return objects.find(objectName) != objects.end();
}
