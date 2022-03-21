#include "StructuredMesh.h"
#include "ArrayStruct.h"
#include "Exception.h"
#include "MeshInfo.h"
#include "UnstructuredMesh.h"
#include "VisitDataTypesToVistle.h"

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
vistle::Object::ptr get(const visit_smart_handle<HandleType::CurvilinearMesh> &meshHandle)
{
    return StructuredMesh::get(meshHandle);
}

namespace StructuredMesh {
vistle::Object::ptr get(const visit_smart_handle<HandleType::CurvilinearMesh> &meshHandle)
{
    int check = simv2_CurvilinearMesh_check(meshHandle);
    if (check == VISIT_OKAY) {
        visit_handle coordHandles[4]; // handles to variable data, 4th entry contains interleaved data depending on
            // coordMode
        int dims[3]{1, 1, 1}; // the x,y,z dimensions
        int ndims, coordMode;
        // no v2check because last visit_handle can be invalid
        if (!simv2_CurvilinearMesh_getCoords(meshHandle, &ndims, dims, &coordMode, &coordHandles[0], &coordHandles[1],
                                             &coordHandles[2], &coordHandles[3])) {
            throw EngineException("makeStructuredMesh: simv2_CurvilinearMesh_getCoords failed");
        }
        assert(dims[0] * dims[1] * dims[2] >= 0);
        auto mesh = make_ptr<vistle::StructuredGrid>((Index)dims[0], (Index)dims[1], (Index)dims[2]);
        if (ndims == 2) {
            std::fill(mesh->z().begin(), mesh->z().end(), 0);
        }

        std::array<vistle::Scalar *, 3> gridCoords{mesh->x().data(), mesh->y().data(), mesh->z().data()};
        detail::fillMeshCoords(coordMode, coordHandles, mesh->getNumCoords(), gridCoords, ndims);
        detail::addGhost(meshHandle, mesh);
        return mesh;
    }
    return nullptr;
}

vistle::Object::ptr getCombinedUnstructured(const MeshInfo &meshInfo, bool vtkFormat)
{
    using namespace UnstructuredMesh;

    size_t totalNumElements = 0, totalNumVerts = 0;
    const int numCorners = meshInfo.dim == 2 ? 4 : 8;
    auto mesh = make_ptr<vistle::UnstructuredGrid>(Index{0}, Index{0}, Index{0});
    std::array<vistle::Scalar *, 3> gridCoords{mesh->x().data(), mesh->y().data(), mesh->z().data()};

    for (size_t iteration = 0; iteration < meshInfo.domains.size; iteration++) {
        visit_handle coordHandles[4]; // handles to variable data, 4th entry contains interleaved data depending on
            // coordMode
        int dims[3]{1, 1, 1}; // the x,y,z dimensions
        int ndims, coordMode;
        detail::getMeshCoord(meshInfo.domains.as<int>()[iteration], meshInfo.name.c_str(), ndims, dims, coordMode,
                             coordHandles);

        size_t numVertices = getNumVertices(dims);
        size_t numElements = getNumElements(dims);

        allocateFields(mesh, totalNumVerts, numVertices, iteration, meshInfo.domains.size, gridCoords, totalNumElements,
                       numElements, numCorners);
        fillStructuredGridConnectivityList(dims, mesh->cl().begin() + totalNumElements * numCorners, totalNumVerts,
                                           vtkFormat ? VtkConListPattern{} : ConnectivityListPattern{});
        detail::fillMeshCoords(coordMode, coordHandles, numVertices, gridCoords, meshInfo.dim);

        gridCoords = {
            gridCoords[0] + numVertices,
            gridCoords[1] + numVertices,
            gridCoords[2] + numVertices,
        };
        totalNumElements += numElements;
        totalNumVerts += numVertices;
    }
    setFinalFildSizes(mesh, totalNumVerts, totalNumElements, numCorners);
    fillRemainingFields(mesh, totalNumElements, numCorners, meshInfo.dim);
    return mesh;
}

namespace detail {

void fillMeshCoords(int coordMode, visit_handle coordHandles[4], size_t numVertices,
                    std::array<vistle::Scalar *, 3> &gridCoords, int dim)
{
    switch (coordMode) {
    case VISIT_COORD_MODE_INTERLEAVED: {
        interleavedFill(coordHandles[3], numVertices, gridCoords, dim);
    } break;
    case VISIT_COORD_MODE_SEPARATE: {
        separateFill(coordHandles, numVertices, gridCoords, dim);
    } break;
    default:
        throw EngineException("coord mode must be interleaved(1) or separate(0), it is " + std::to_string(coordMode));
    }
}

void getMeshCoord(int currDomain, const char *name, int &ndims, int dims[3], int &coordMode,
                  visit_handle coordHandles[4])
{
    visit_handle meshHandle = v2check(simv2_invoke_GetMesh, currDomain, name);
    // no v2check because last visit_handle can be invalid
    if (!simv2_CurvilinearMesh_getCoords(meshHandle, &ndims, dims, &coordMode, &coordHandles[0], &coordHandles[1],
                                         &coordHandles[2], &coordHandles[3])) {
        throw EngineException("makeStructuredMesh: simv2_CurvilinearMesh_getCoords failed");
    }
}

void separateFill(visit_handle coordHandles[4], int numCoords, std::array<vistle::Scalar *, 3> &meshCoords, int dim)
{
    for (int i = 0; i < dim; ++i) {
        auto meshArray = getVariableData(coordHandles[i]);
        if (meshArray.size != static_cast<size_t>(numCoords)) {
            throw EngineException("get(): received points in separate mesh do not match the mesh's dimension ") << i;
        }
        transformArray(meshArray, meshCoords[i]);
    }
}

void interleavedFill(visit_handle coordHandle, int numCoords, const std::array<vistle::Scalar *, 3> &meshCoords,
                     int dim)
{
    auto meshArray = getVariableData(coordHandle);
    if (meshArray.size != static_cast<size_t>(numCoords * dim)) {
        throw EngineException("get(): received points in interleaved mesh do not match the mesh's dimensions");
    }

    transformInterleavedArray(meshArray.data, meshCoords, numCoords, dataTypeToVistle(meshArray.type), dim);
}

void addGhost(const visit_handle &meshHandle, std::shared_ptr<vistle::StructuredGrid> mesh)
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

} // namespace detail
} // namespace StructuredMesh
} // namespace libsim
} // namespace insitu
} // namespace vistle
