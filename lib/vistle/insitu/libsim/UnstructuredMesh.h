#ifndef VISTLE_INSITU_LIBSIM_UNSTRUCTUREDMESH_H
#define VISTLE_INSITU_LIBSIM_UNSTRUCTUREDMESH_H

#include "ArrayStruct.h"

#include <vistle/core/index.h>
#include <vistle/core/object.h>
#include "SmartHandle.h"

#include <memory>
#include <array>

class visit_handle;
namespace vistle {
class UnstructuredGrid;
namespace insitu {
namespace message {
class SyncShmIDs;
}

namespace libsim {
vistle::Object::ptr get(const visit_smart_handle<HandleType::UnstructuredMesh> &meshHandle);
namespace UnstructuredMesh {

vistle::Object::ptr get(const visit_smart_handle<HandleType::UnstructuredMesh> &meshHandle);

namespace detail {
void SeparateAllocAndFill(int dim, const visit_handle coordHandles[4], std::shared_ptr<vistle::UnstructuredGrid> grid);
void InterleavedAllocAndFill(const visit_handle &coordHandle, std::shared_ptr<vistle::UnstructuredGrid> grid, int dim);
void addGhost(const visit_handle &meshHandle, std::shared_ptr<vistle::UnstructuredGrid> grid);
void fillTypeConnAndElemLists(const visit_handle &meshHandle, std::shared_ptr<vistle::UnstructuredGrid> grid);
struct connListData {
    Array<HandleType::Coords> data;
    int numElements = 0;
};
connListData getConListFromSim(const visit_handle &meshHandle);
} // namespace detail

size_t getNumVertices(int dims[3]);
void preventNull(int dims[3], void *data[3]);
size_t getNumElements(int dims[3]);
void allocateFields(std::shared_ptr<vistle::UnstructuredGrid> grid, size_t totalNumVerts, size_t numVertices,
                    size_t iteration, int numIterations, std::array<vistle::Scalar *, 3> &gridCoords,
                    size_t totalNumElements, size_t numElements, int numCorners);
void setFinalFildSizes(std::shared_ptr<vistle::UnstructuredGrid> grid, size_t totalNumVerts, size_t totalNumElements,
                       int numCorners);
void fillRemainingFields(std::shared_ptr<vistle::UnstructuredGrid> grid, size_t totalNumElements, int numCorners,
                         int dim);


struct ConnectivityListPattern {
    ConnectivityListPattern();
    ConnectivityListPattern(vistle::Index (*getVertexIndex)(vistle::Index, vistle::Index, vistle::Index,
                                                            const vistle::Index[3]),
                            std::array<int, 3> loopOrder);
    vistle::Index (*vertexIndex)(vistle::Index, vistle::Index, vistle::Index, const vistle::Index[3]) = nullptr;
    std::array<int, 3> order = {{0, 1, 2}}; //Vistle's loop order is x, y, z
};
struct VtkConListPattern: ConnectivityListPattern {
    VtkConListPattern();
};
void fillStructuredGridConnectivityList(const int *dims, vistle::Index *connectivityList,
                                        vistle::Index startOfGridIndex,
                                        ConnectivityListPattern pattern = ConnectivityListPattern{});
} // namespace UnstructuredMesh
} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif
