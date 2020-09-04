#include "StructuredMesh.h"
#include "Exeption.h"
#include "VisitDataTypesToVistle.h"
#include "UnstructuredMesh.h"
#include "MeshInfo.h"

#include <vistle/insitu/core/transformArray.h>
#include <vistle/insitu/message/SyncShmIDs.h>

#include <vistle/core/structuredgrid.h>
#include <vistle/core/unstr.h>

#include <vistle/insitu/libsim/libsimInterface/CurvilinearMesh.h>
#include <vistle/insitu/libsim/libsimInterface/VariableData.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataInterfaceRuntime.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>



namespace vistle {
namespace insitu {
namespace libsim {
namespace StructuredMesh {
vistle::StructuredGrid::ptr get(const visit_handle& meshHandle, message::SyncShmIDs& creator)
{
    int check = simv2_CurvilinearMesh_check(meshHandle);
    if (check == VISIT_OKAY) {
        visit_handle coordHandles[4]; //handles to variable data, 4th entry contains interleaved data depending on coordMode
        int dims[3]{ 1,1,1 }; //the x,y,z dimensions
        int ndims, coordMode;
        //no v2check because last visit_handle can be invalid
        if (!simv2_CurvilinearMesh_getCoords(meshHandle, &ndims, dims, &coordMode, &coordHandles[0], &coordHandles[1], &coordHandles[2], &coordHandles[3])) {
            throw EngineExeption("makeStructuredMesh: simv2_CurvilinearMesh_getCoords failed");
        }
        vistle::StructuredGrid::ptr mesh = creator.createVistleObject<vistle::StructuredGrid>(dims[0], dims[1], dims[2]);
        if (ndims == 2) {
            std::fill(mesh->z().begin(), mesh->z().end(), 0);
        }

        std::array<float*, 3> gridCoords{ mesh->x().data() ,mesh->y().data() ,mesh->z().data() };
        detail::fillMeshCoords(coordMode, coordHandles, mesh->getNumCoords(), gridCoords, ndims);
        detail::addGhost(meshHandle, mesh);
    }
    return nullptr;
}

std::shared_ptr<vistle::UnstructuredGrid> getCombinedUnstructured(const MeshInfo& meshInfo, message::SyncShmIDs& creator, bool vtkFormat)
{
    using namespace UnstructuredMesh;

    size_t totalNumElements = 0, totalNumVerts = 0;
    const int numCorners = meshInfo.dim == 2 ? 4 : 8;
    vistle::UnstructuredGrid::ptr mesh = creator.createVistleObject<vistle::UnstructuredGrid>(0, 0, 0);
    std::array<float*, 3> gridCoords{ mesh->x().data() ,mesh->y().data() ,mesh->z().data() };

    for (size_t iteration = 0; iteration < meshInfo.numDomains; iteration++) {

        visit_handle coordHandles[4]; //handles to variable data, 4th entry contains interleaved data depending on coordMode
        int dims[3]{ 1,1,1 }; //the x,y,z dimensions
        int ndims, coordMode;
        detail::getMeshCoord(meshInfo.domains[iteration], meshInfo.name, ndims, dims, coordMode, coordHandles);

        size_t numVertices = getNumVertices(dims);
        size_t numElements = getNumElements(dims);

        allocateFields(mesh, totalNumVerts, numVertices, iteration, meshInfo.numDomains, gridCoords, totalNumElements, numElements, numCorners);
        fillStructuredGridConnectivityList(dims, mesh->cl().begin() + totalNumElements * numCorners, totalNumVerts, vtkFormat ? VtkConListPattern{} : ConnectivityListPattern{});
        detail::fillMeshCoords(coordMode, coordHandles, numVertices, gridCoords, meshInfo.dim);

        gridCoords = { gridCoords[0] + numVertices, gridCoords[1] + numVertices, gridCoords[2] + numVertices, };
        totalNumElements += numElements;
        totalNumVerts += numVertices;

    }
    setFinalFildSizes(mesh, totalNumVerts, totalNumElements, numCorners);
    fillRemainingFields(mesh, totalNumElements, numCorners, meshInfo.dim);
    return mesh;
}

void Test(message::SyncShmIDs& creator, bool vtkFormat)
{
}

namespace detail {

void fillMeshCoords(int coordMode, visit_handle  coordHandles[4], size_t numVertices, std::array<float*, 3Ui64>& gridCoords, int dim)
{
    switch (coordMode) {
    case VISIT_COORD_MODE_INTERLEAVED:
    {
        interleavedFill(coordHandles[3], numVertices, gridCoords, dim);
    }
    break;
    case VISIT_COORD_MODE_SEPARATE:
    {
        separateFill(coordHandles, numVertices, gridCoords, dim);
    }
    break;
    default:
        throw EngineExeption("coord mode must be interleaved(1) or separate(0), it is " + std::to_string(coordMode));
    }
}

void getMeshCoord(int currDomain, const char* name, int& ndims, int dims[3], int& coordMode, visit_handle coordHandles[4])
{
    visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, name);
    //no v2check because last visit_handle can be invalid
    if (!simv2_CurvilinearMesh_getCoords(meshHandle, &ndims, dims, &coordMode, &coordHandles[0], &coordHandles[1], &coordHandles[2], &coordHandles[3])) {
        throw EngineExeption("makeStructuredMesh: simv2_CurvilinearMesh_getCoords failed");
    }
}

void separateFill(visit_handle  coordHandles[4], int numCoords, std::array<float*, 3Ui64>& meshCoords, int dim)
{
    int owner{}, dataType{}, nComps{}, nTuples{};
    void* data{};
    for (int i = 0; i < dim; ++i) {
        v2check(simv2_VariableData_getData, coordHandles[i], owner, dataType, nComps, nTuples, data);
        if (nTuples != numCoords) {
            throw EngineExeption("get(): received points in separate mesh do not match the mesh's dimension ") << i;
        }
        transformArray(data, meshCoords[i], nTuples, dataTypeToVistle(dataType));
    }
}

void interleavedFill(visit_handle  coordHandle, int numCoords, const std::array<float*, 3Ui64>& meshCoords, int dim)
{
    int owner{}, dataType{}, nComps{}, nTuples{};
    void* data{};
    v2check(simv2_VariableData_getData, coordHandle, owner, dataType, nComps, nTuples, data);
    if (nTuples != numCoords * dim) {
        throw EngineExeption("get(): received points in interleaved mesh do not match the mesh's dimensions");
    }

    transformInterleavedArray(data, meshCoords, numCoords, dataTypeToVistle(dataType), dim);
}

void addGhost(const visit_handle& meshHandle, std::shared_ptr<vistle::StructuredGrid> mesh)
{
    std::array<int, 3> min, max;
    v2check(simv2_CurvilinearMesh_getRealIndices, meshHandle, min.data(), max.data());

    for (size_t i = 0; i < 3; i++) {
        assert(min[i] >= 0);
        int numTop = mesh->getNumDivisions(i) - 1 - max[i];
        assert(numTop >= 0);
        mesh->setNumGhostLayers(i, vistle::StructuredGridBase::GhostLayerPosition::Bottom, min[i]);
        mesh->setNumGhostLayers(i, vistle::StructuredGridBase::GhostLayerPosition::Top, numTop);
    }
}

}//detail
}//StructuredMesh
}//libsim
}//insitu
}//vistle


