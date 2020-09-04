#include "RectilinearMesh.h"
#include "Exeption.h"
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
namespace insitu{
namespace libsim {
namespace RectilinearMesh {

vistle::RectilinearGrid::ptr get(const visit_handle &meshHandle, message::SyncShmIDs& creator)
{
    if (simv2_RectilinearMesh_check(meshHandle) == VISIT_OKAY) {
        int owners[3], dataTypes[3], dims[3]{ 1,1,1 };
        void* data[3]{};
        detail::getMeshFromSim(meshHandle, owners, dataTypes, dims, data);
        auto mesh = detail::makeVistleMesh(data, dims, dataTypes, creator);
        detail::addGhost(meshHandle, mesh);
        return mesh;
    }
    return nullptr;

}

vistle::UnstructuredGrid::ptr getCombinedUnstructured(const MeshInfo& meshInfo, message::SyncShmIDs& creator, bool vtkFormat)
{
    using namespace UnstructuredMesh;
    size_t totalNumElements = 0, totalNumVerts = 0;
    const int numCorners = meshInfo.dim == 2 ? 4 : 8;
    vistle::UnstructuredGrid::ptr mesh = creator.createVistleObject<vistle::UnstructuredGrid>(0, 0, 0);
    std::array<float*, 3> gridCoords{ mesh->x().data() ,mesh->y().data() ,mesh->z().data() };

    for (size_t iteration = 0; iteration < meshInfo.numDomains; iteration++) {
        
        visit_handle coordHandles[3]; 
        int owners[3], dataTypes[3], dims[3]{ 1,1,1 };
        void* data[3]{};
        visit_handle meshHandle = v2check(simv2_invoke_GetMesh, meshInfo.domains[iteration], meshInfo.name);
        detail::getMeshFromSim(meshHandle, owners, dataTypes, dims, data);
        
        size_t numVertices = getNumVertices(dims);
        size_t numElements = getNumElements(dims);
        preventNull(dims, data);
        allocateFields(mesh, totalNumVerts, numVertices, iteration, meshInfo.numDomains, gridCoords, totalNumElements, numElements, numCorners);

        if (vtkFormat) {
            expandRectilinearToVTKStructured(data, dataTypeToVistle(dataTypes[0]), dims, gridCoords);
            fillStructuredGridConnectivityList(dims, mesh->cl().begin() + totalNumElements * numCorners, totalNumVerts, VtkConListPattern{});

        }
        else {
            expandRectilinearToStructured(data, dataTypeToVistle(dataTypes[0]), dims, gridCoords);
            fillStructuredGridConnectivityList(dims, mesh->cl().begin() + totalNumElements * numCorners, totalNumVerts);

        }

        gridCoords = { gridCoords[0] + numVertices, gridCoords[1] + numVertices, gridCoords[2] + numVertices, };
        totalNumElements += numElements;
        totalNumVerts += numVertices;

    }
    setFinalFildSizes(mesh, totalNumVerts, totalNumElements, numCorners);
    fillRemainingFields(mesh, totalNumElements, numCorners, meshInfo.dim);
    return mesh;
}


namespace detail {

vistle::RectilinearGrid::ptr makeVistleMesh(void* data[3], int sizes[3], int dataType[3], message::SyncShmIDs& creator)
{
    vistle::RectilinearGrid::ptr mesh = creator.createVistleObject<vistle::RectilinearGrid>(sizes[0], sizes[1], sizes[2]);

    for (size_t i = 0; i < 3; ++i) {
        if (data[i]) {
            transformArray(data[i], mesh->coords(i).begin(), sizes[i], dataTypeToVistle(dataType[i]));
        }
        else {
            mesh->coords(i)[0] = 0;
        }
    }
    return mesh;
}

void addGhost(const visit_handle& meshHandle, std::shared_ptr<vistle::RectilinearGrid> mesh)
{
    std::array<int, 3> min, max;
    v2check(simv2_RectilinearMesh_getRealIndices, meshHandle, min.data(), max.data());

    for (size_t i = 0; i < 3; i++) {
        assert(min[i] >= 0);
        int numTop = mesh->getNumDivisions(i) - 1 - max[i];
        assert(numTop >= 0);
        mesh->setNumGhostLayers(i, vistle::StructuredGridBase::GhostLayerPosition::Bottom, min[i]);
        mesh->setNumGhostLayers(i, vistle::StructuredGridBase::GhostLayerPosition::Top, numTop);
    }
}

void getMeshFromSim(const visit_handle& meshHandle, int owner[3], int dataType[3], int nTuples[], void* data[])
{
    visit_handle coordHandles[3]; //handles to variable data
    int ndims;
    v2check(simv2_RectilinearMesh_getCoords, meshHandle, &ndims, &coordHandles[0], &coordHandles[1], &coordHandles[2]);
    std::array<int, 3> nComps{};
    assert(ndims <= 3);
    for (int i = 0; i < ndims; ++i) {
        v2check(simv2_VariableData_getData, coordHandles[i], owner[i], dataType[i], nComps[i], nTuples[i], data[i]);
        if (dataType[i] != dataType[0]) {
            throw EngineExeption{ "mesh data type must be consistent within a domain" };
        }
    }
}

}//detail
}//RectilinearMesh
}//libsim
}//insitu
}//vistle

