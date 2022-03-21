#include "RectilinearMesh.h"
#include "Exception.h"
#include "VisitDataTypesToVistle.h"
#include "UnstructuredMesh.h"
#include "MeshInfo.h"

#include <vistle/core/rectilineargrid.h>
#include <vistle/core/unstr.h>

#include <vistle/insitu/core/transformArray.h>
#include <vistle/insitu/message/SyncShmIDs.h>

#include <vistle/insitu/libsim/libsimInterface/RectilinearMesh.h>
#include <vistle/insitu/libsim/libsimInterface/VariableData.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataInterfaceRuntime.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>


namespace vistle {
namespace insitu {
namespace libsim {
std::shared_ptr<Object> get(const visit_smart_handle<HandleType::RectilinearMesh> &meshHandle)
{
    return RectilinearMesh::get(meshHandle);
}

namespace RectilinearMesh {

Object::ptr get(const visit_smart_handle<HandleType::RectilinearMesh> &meshHandle)
{
    if (simv2_RectilinearMesh_check(meshHandle) == VISIT_OKAY) {
        auto meshArray = detail::getMeshFromSim(meshHandle);
        auto mesh = detail::makeVistleMesh(meshArray);
        detail::addGhost(meshHandle, mesh);
        return mesh;
    }
    return nullptr;
}

Object::ptr getCombinedUnstructured(const MeshInfo &meshInfo, bool vtkFormat)
{
    using namespace UnstructuredMesh;
    size_t totalNumElements = 0, totalNumVerts = 0;
    const int numCorners = meshInfo.dim == 2 ? 4 : 8;
    auto mesh = make_ptr<UnstructuredGrid>(Index{0}, Index{0}, Index{0});
    std::array<Scalar *, 3> gridCoords{mesh->x().data(), mesh->y().data(), mesh->z().data()};

    for (size_t iteration = 0; iteration < meshInfo.domains.size; iteration++) {
        visit_handle meshHandle =
            v2check(simv2_invoke_GetMesh, meshInfo.domains.as<int>()[iteration], meshInfo.name.c_str());
        auto meshArrays = detail::getMeshFromSim(meshHandle);

        int dims[3]{1, 1, 1};
        void *data[3];
        for (size_t i = 0; i < 3; i++) {
            dims[i] = meshArrays[i].dim;
            data[i] = meshArrays[i].data;
        }
        size_t numVertices = getNumVertices(dims);
        size_t numElements = getNumElements(dims);
        preventNull(dims, data);
        allocateFields(mesh, totalNumVerts, numVertices, iteration, meshInfo.domains.size, gridCoords, totalNumElements,
                       numElements, numCorners);

        if (vtkFormat) {
            expandRectilinearToVTKStructured(data, dataTypeToVistle(meshArrays[0].type), dims, gridCoords);
            fillStructuredGridConnectivityList(dims, mesh->cl().begin() + totalNumElements * numCorners, totalNumVerts,
                                               VtkConListPattern{});

        } else {
            expandRectilinearToStructured(data, dataTypeToVistle(meshArrays[0].type), dims, gridCoords);
            fillStructuredGridConnectivityList(dims, mesh->cl().begin() + totalNumElements * numCorners, totalNumVerts);
        }

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

RectilinearGrid::ptr makeVistleMesh(const std::array<Array<HandleType::Coords>, 3> &meshData)
{
    auto mesh = make_ptr<RectilinearGrid>((Index)meshData[0].size, (Index)meshData[1].size, (Index)meshData[2].size);
    for (size_t i = 0; i < 3; ++i) {
        if (meshData[i].data) {
            transformArray(meshData[i], mesh->coords(i).begin());
        } else {
            mesh->coords(i)[0] = 0;
        }
    }
    return mesh;
}

void addGhost(const visit_handle &meshHandle, std::shared_ptr<RectilinearGrid> mesh)
{
    std::array<int, 3> min, max;
    v2check(simv2_RectilinearMesh_getRealIndices, meshHandle, min.data(), max.data());

    for (size_t i = 0; i < 3; i++) {
        assert(min[i] >= 0);
        int numTop = mesh->getNumDivisions(i) - 1 - max[i];
        assert(numTop >= 0);
        mesh->setNumGhostLayers(i, StructuredGridBase::GhostLayerPosition::Bottom, min[i]);
        mesh->setNumGhostLayers(i, StructuredGridBase::GhostLayerPosition::Top, numTop);
    }
}

std::array<Array<HandleType::Coords>, 3>
getMeshFromSim(const visit_smart_handle<HandleType::RectilinearMesh> &meshHandle)
{
    visit_smart_handle<HandleType::Coords> coordHandles[3]; //handles to variable data
    int ndims;
    v2check(simv2_RectilinearMesh_getCoords, meshHandle, &ndims, &coordHandles[0], &coordHandles[1], &coordHandles[2]);
    std::array<Array<HandleType::Coords>, 3> meshData;
    assert(ndims <= 3);
    for (int i = 0; i < ndims; ++i) {
        meshData[i] = getVariableData(coordHandles[i]);
        if (meshData[i].type != meshData[0].type) {
            throw EngineException{"mesh data type must be consistent within a domain"};
        }
    }
    return meshData;
}

} // namespace detail
} // namespace RectilinearMesh
} // namespace libsim
} // namespace insitu
} // namespace vistle
