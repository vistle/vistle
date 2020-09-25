#include "UnstructuredMesh.h"
#include "Exeption.h"
#include "VisitDataTypesToVistle.h"
#include "VertexTypesToVistle.h"
#include "ArrayStruct.h"

#include <vistle/insitu/core/transformArray.h>

#include <vistle/insitu/message/SyncShmIDs.h>

#include <vistle/insitu/libsim/libsimInterface/UnstructuredMesh.h>
#include <vistle/insitu/libsim/libsimInterface/VariableData.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataInterfaceRuntime.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>

#include <vistle/core/unstr.h>

#include <iostream>

namespace vistle {
namespace insitu {
namespace libsim {
namespace UnstructuredMesh {
vistle::Object::ptr get(const visit_handle& meshHandle, message::SyncShmIDs& creator)
{
    if (simv2_UnstructuredMesh_check(meshHandle) == VISIT_OKAY) {
        vistle::UnstructuredGrid::ptr mesh = creator.createVistleObject<vistle::UnstructuredGrid>(0, 0, 0);
        detail::fillTypeConnAndElemLists(meshHandle, mesh);


        visit_handle coordHandles[4]; //handles to variable data pos 3 is for interleaved data
        int ndims;
        int coordMode;
        v2check(simv2_UnstructuredMesh_getCoords, meshHandle, &ndims, &coordMode, &coordHandles[0], &coordHandles[1], &coordHandles[2], &coordHandles[3]);

        switch (coordMode) {
        case VISIT_COORD_MODE_INTERLEAVED:
        {
            detail::InterleavedAllocAndFill(coordHandles[3], mesh, ndims);
        }
        break;
        case VISIT_COORD_MODE_SEPARATE:
        {
            detail::SeparateAllocAndFill(ndims, coordHandles, mesh);
        }
        break;
        default:
            throw EngineExeption("coord mode must be interleaved(1) or separate(0), it is " + std::to_string(coordMode));
        }
        detail::addGhost(meshHandle, mesh);
        return mesh;
    }
    return nullptr;
}

namespace detail {

void SeparateAllocAndFill(int dim, const visit_handle coordHandles[4], std::shared_ptr<vistle::UnstructuredGrid> grid)
{
    for (int i = 0; i < dim; ++i) {
        auto a = getVariableData(coordHandles[i]);
        grid->x(i).resize(a.size);
        transformArray(a, grid->x(i).data());
    }
}

void InterleavedAllocAndFill(const visit_handle &coordHandle, std::shared_ptr<vistle::UnstructuredGrid> grid, int dim)
{
    auto a = getVariableData(coordHandle);
    std::array<vistle::Scalar*, 3> gridCoords;
    for (size_t i = 0; i < static_cast<size_t>(dim); ++i)
    {
        grid->x(i).resize(a.size);
        gridCoords[i] = grid->x(i).data();
    }
    transformInterleavedArray(a.data, gridCoords, a.size, dataTypeToVistle(a.type), dim);
}

void addGhost(const visit_handle& meshHandle, vistle::UnstructuredGrid::ptr grid)
{
    visit_handle ghostCellHandle;
    v2check(simv2_UnstructuredMesh_getGhostCells, meshHandle, &ghostCellHandle);
    if (ghostCellHandle != VISIT_INVALID_HANDLE)
    {
        auto ghostArray = getVariableData(ghostCellHandle);
        if (ghostArray.type != VISIT_DATATYPE_CHAR)
        {
            EngineExeption ex("expectet ghostCellData of type char, type is ");
            ex << ghostArray.type;
            throw ex;
        }
        char* c = static_cast<char*>(ghostArray.data);
        for (size_t i = 0; i < ghostArray.size; ++i)
        {
            if (c[i])
            {
                grid->tl()[i] |= vistle::UnstructuredGrid::GHOST_BIT;
            }
        }
    }
}

void fillTypeConnAndElemLists(const visit_handle& meshHandle, vistle::UnstructuredGrid::ptr mesh)
{
    auto connListData = getConListFromSim(meshHandle);

    mesh->tl().reserve(connListData.numElements);
    mesh->el().reserve(connListData.numElements);
    size_t idx = 0;

    size_t elemIndex = 0;
    for (int i = 0; i < connListData.numElements; ++i)
    {
        auto type = static_cast<int*>(connListData.data.data)[idx];
        ++idx;
        auto elemType = libsim::vertexTypeToVistle(type);
        if (elemType == vistle::UnstructuredGrid::Type::NONE)
        {
            std::cerr << "UnstructuredMesh warning: type " << type << " is not converted to a vistle type...returning" << std::endl;
            return;
        }

        elemIndex += libsim::getNumVertices(elemType);

        if (idx + libsim::getNumVertices(elemType) > connListData.data.size)
        {
            std::cerr << "element " << i << " with idx = " << idx << " and type " << elemType << "with typesize" << libsim::getNumVertices(elemType) << " is outside of given elementList data!" << std::endl;
            break;
        }
        mesh->el().push_back(elemIndex);
        mesh->tl().push_back(elemType);

        for (size_t i = 0; i < libsim::getNumVertices(elemType); i++)
        {
            mesh->cl().push_back(static_cast<int*>(connListData.data.data)[idx]);
            ++idx;
        }
    }
}

connListData getConListFromSim(const visit_handle &meshHandle) {
    visit_handle connListHandle;
    connListData cld;
    v2check(simv2_UnstructuredMesh_getConnectivity, meshHandle, &cld.numElements, &connListHandle);
    cld.data = getVariableData(connListHandle);
    if (cld.data.type != VISIT_DATATYPE_INT)
    {
        throw EngineExeption("element list is not of expected type (int)");
    }
    return cld;
}

}//detail

size_t getNumElements(int  dims[3])
{
    size_t numElements = 1;
    for (size_t i = 0; i < 3; i++)
    {
        numElements *= std::max(dims[i] - 1, 1);
    }
    return numElements;
}

size_t getNumVertices(int  dims[3])
{
    size_t numVerts = 1;
    for (size_t i = 0; i < 3; i++)
    {
        numVerts *= dims[i];
    }
    return numVerts;
}

void preventNull(int  dims[3], void* data[3])
{
    static double zero = 0;
    for (size_t i = 0; i < 3; i++)
    {
        dims[i] = std::max(dims[i], 1);
        if (!data[i])
        {
            data[i] = &zero;
        }
    }
}

void allocateFields(vistle::UnstructuredGrid::ptr grid, size_t totalNumVerts, size_t numVertices, size_t iteration, int numIterations, std::array<float*, 3>& gridCoords, size_t totalNumElements, size_t numElements, int numCorners)
{
    // reserve memory for the arrays, /assume we have the same sub-grid size for the rest to reduce the amout of re-allocations
    if (grid->x().size() < totalNumVerts + numVertices) {
        auto newSize = totalNumVerts + numVertices * (numIterations - iteration);
        grid->setSize(newSize);
        gridCoords = { grid->x().begin() + totalNumVerts ,grid->y().begin() + totalNumVerts ,grid->z().begin() + totalNumVerts };
    }
    if (grid->cl().size() < (totalNumElements + numElements) * numCorners) {
        grid->cl().resize((totalNumElements + numElements * (numIterations - iteration)) * numCorners);
    }
}

void setFinalFildSizes(vistle::UnstructuredGrid::ptr grid, size_t totalNumVerts, size_t totalNumElements, int numCorners)
{
    assert(grid->getSize() >= totalNumVerts);
    grid->setSize(totalNumVerts);

    assert(grid->cl().size() >= totalNumElements * numCorners);
    grid->cl().resize(totalNumElements * numCorners);

    grid->tl().resize(totalNumElements);
    grid->el().resize(totalNumElements + 1);
}

void fillRemainingFields(vistle::UnstructuredGrid::ptr grid, size_t totalNumElements, int numCorners, int dim)
{
    std::fill(grid->tl().begin(), grid->tl().end(), dim == 2 ? (const vistle::Byte)vistle::UnstructuredGrid::QUAD : (const vistle::Byte)vistle::UnstructuredGrid::HEXAHEDRON);
    for (size_t i = 0; i < totalNumElements + 1; i++) {
        grid->el()[i] = numCorners * i;
    }

    if (dim == 2) {
        std::fill(grid->z().begin(), grid->z().end(), 0);
    }
}

void fillStructuredGridConnectivityList(const int* dims, vistle::Index* connectivityList, vistle::Index startOfGridIndex, ConnectivityListPattern pattern) {
    // construct connectivity list (all hexahedra)
    using namespace vistle;
    Index numVert[3];
    Index numElements[3];
    for (size_t i = 0; i < 3; ++i) {
        numVert[i] = static_cast<Index>(dims[i]);
        numElements[i] = std::max(numVert[i] - 1, Index{ 1 });
    }
    auto& o = pattern.order;
    if (dims[2] > 1) {
        // 3-dim
        Index el = 0;
        std::array<Index, 3> i{ 0,0,0 };
        for (i[o[0]] = 0; i[o[0]] < numElements[o[0]]; ++i[o[0]]) {
            for (i[o[1]] = 0; i[o[1]] < numElements[o[1]]; ++i[o[1]]) {
                for (i[o[2]] = 0; i[o[2]] < numElements[o[2]]; ++i[o[2]]) {
                    const Index baseInsertionIndex = el * 8;
                    connectivityList[baseInsertionIndex + 0] = startOfGridIndex + pattern.vertexIndex(i[0], i[1], i[2], numVert);           // 0       7 -------- 6
                    connectivityList[baseInsertionIndex + 1] = startOfGridIndex + pattern.vertexIndex(i[0] + 1, i[1], i[2], numVert);           // 1      /|         /|
                    connectivityList[baseInsertionIndex + 2] = startOfGridIndex + pattern.vertexIndex(i[0] + 1, i[1] + 1, i[2], numVert);           // 2     / |        / |
                    connectivityList[baseInsertionIndex + 3] = startOfGridIndex + pattern.vertexIndex(i[0], i[1] + 1, i[2], numVert);           // 3    4 -------- 5  |
                    connectivityList[baseInsertionIndex + 4] = startOfGridIndex + pattern.vertexIndex(i[0], i[1], i[2] + 1, numVert);           // 4    |  3-------|--2
                    connectivityList[baseInsertionIndex + 5] = startOfGridIndex + pattern.vertexIndex(i[0] + 1, i[1], i[2] + 1, numVert);           // 5    | /        | /
                    connectivityList[baseInsertionIndex + 6] = startOfGridIndex + pattern.vertexIndex(i[0] + 1, i[1] + 1, i[2] + 1, numVert);           // 6    |/         |/
                    connectivityList[baseInsertionIndex + 7] = startOfGridIndex + pattern.vertexIndex(i[0], i[1] + 1, i[2] + 1, numVert);           // 7    0----------1

                    ++el;
                }
            }
        }
    }
    else if (dims[1] > 1) {
        // 2-dim
        Index el = 0;
        std::array<Index, 3> i{ 0,0,0 };
        for (i[o[0]] = 0; i[o[0]] < numElements[o[0]]; ++i[o[0]]) {
            for (i[o[1]] = 0; i[o[1]] < numElements[o[1]]; ++i[o[1]]) {
                for (i[o[2]] = 0; i[o[2]] < numElements[o[2]]; i[o[2]]++)
                {
                    const Index baseInsertionIndex = el * 4;
                    connectivityList[baseInsertionIndex + 0] = startOfGridIndex + pattern.vertexIndex(i[0], i[1], 0, numVert);
                    connectivityList[baseInsertionIndex + 1] = startOfGridIndex + pattern.vertexIndex(i[0] + 1, i[1], 0, numVert);
                    connectivityList[baseInsertionIndex + 2] = startOfGridIndex + pattern.vertexIndex(i[0] + 1, i[1] + 1, 0, numVert);
                    connectivityList[baseInsertionIndex + 3] = startOfGridIndex + pattern.vertexIndex(i[0], i[1] + 1, 0, numVert);
                    ++el;
                }
            }
        }
    }
}

VtkConListPattern::VtkConListPattern()
    :ConnectivityListPattern({ VTKVertexIndex, {2,1,0} })
{
}

ConnectivityListPattern::ConnectivityListPattern()
    : vertexIndex(vistle::StructuredGridBase::vertexIndex)
    , order({ 0,1,2 })
{
}

ConnectivityListPattern::ConnectivityListPattern(vistle::Index(*getVertexIndex)(vistle::Index, vistle::Index, vistle::Index, const vistle::Index[3]), std::array<int, 3> loopOrder)
    : vertexIndex(getVertexIndex)
    , order(loopOrder)
{
}

}//UnstructuredMesh

}//libsim
}//insitu
}//vistle









