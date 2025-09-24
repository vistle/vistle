#include "PlaneClip.h"

#include "vistle/core/index.h"
#include "vistle/core/vector.h"

#include <algorithm>
#include <tuple>

using namespace vistle;

PlaneClip::PlaneClip(Triangles::const_ptr grid, IsoDataFunctor decider)
: m_coord(grid)
, m_tri(grid)
, m_decider(decider)
, haveCornerList(false)
, el(nullptr)
, cl(nullptr)
, x(nullptr)
, y(nullptr)
, z(nullptr)
, d(nullptr)
, out_cl(nullptr)
, out_el(nullptr)
, out_x(nullptr)
, out_y(nullptr)
, out_z(nullptr)
, out_d(nullptr)
{
    if (grid->getNumCorners() > 0) {
        haveCornerList = true;
        cl = grid->cl().data();
    }
    x = grid->x().data();
    y = grid->y().data();
    z = grid->z().data();

    m_outTri = Triangles::ptr(new Triangles(Object::Initialized));
    m_outTri->setMeta(grid->meta());
    m_outCoords = m_outTri;
}

PlaneClip::PlaneClip(Polygons::const_ptr grid, IsoDataFunctor decider)
: m_coord(grid)
, m_poly(grid)
, m_decider(decider)
, haveCornerList(true)
, el(nullptr)
, cl(nullptr)
, x(nullptr)
, y(nullptr)
, z(nullptr)
, d(nullptr)
, out_cl(nullptr)
, out_el(nullptr)
, out_x(nullptr)
, out_y(nullptr)
, out_z(nullptr)
, out_d(nullptr)
{
    el = grid->el().data();
    cl = grid->cl().data();
    x = grid->x().data();
    y = grid->y().data();
    z = grid->z().data();

    m_outPoly = Polygons::ptr(new Polygons(Object::Initialized));
    m_outPoly->setMeta(grid->meta());
    m_outCoords = m_outPoly;
}

void PlaneClip::addData(Object::const_ptr obj)
{
    m_data.push_back(obj);
    auto data = Vec<Scalar, 1>::as(obj);
    if (data) {
        d = data->x().data();

        m_outData = Vec<Scalar>::ptr(new Vec<Scalar>(Object::Initialized));
        m_outData->setMeta(data->meta());
    }
}

void PlaneClip::prepareTriangles(std::vector<Index> &outIdxCorner, std::vector<Index> &outIdxCoord,
                                 const Index &numCoordsIn, const Index &numElem)
{
    outIdxCorner[0] = 0;
    outIdxCoord[0] = numCoordsIn;
#pragma omp parallel for schedule(dynamic)
    for (ssize_t elem = 0; elem < ssize_t(numElem); ++elem) {
        Index numCorner = 0, numCoord = 0;
        processTriangle(elem, numCorner, numCoord, true);

        outIdxCorner[elem + 1] = numCorner;
        outIdxCoord[elem + 1] = numCoord;
    }

    for (Index elem = 0; elem < numElem; ++elem) {
        outIdxCoord[elem + 1] += outIdxCoord[elem];
        outIdxCorner[elem + 1] += outIdxCorner[elem];
    }

    Index numCoord = outIdxCoord[numElem];
    Index numCorner = outIdxCorner[numElem];

    if (haveCornerList) {
        m_outTri->cl().resize(numCorner);
        out_cl = m_outTri->cl().data();
    }

    m_outTri->setSize(numCoord);
    out_x = m_outTri->x().data();
    out_y = m_outTri->y().data();
    out_z = m_outTri->z().data();

    if (m_outData) {
        m_outData->x().resize(numCoord);
        out_d = m_outData->x().data();
    }
}

void PlaneClip::preparePolygons(std::vector<Index> &outIdxPoly, std::vector<Index> &outIdxCorner,
                                std::vector<Index> &outIdxCoord, const Index &numCoordsIn, const Index &numElem)
{
#pragma omp parallel for schedule(dynamic)
    for (ssize_t elem = 0; elem < ssize_t(numElem); ++elem) {
        Index numPoly = 0, numCorner = 0, numCoord = 0;
        processPolygon(elem, numPoly, numCorner, numCoord, true);
        outIdxPoly[elem + 1] = numPoly;
        outIdxCorner[elem + 1] = numCorner;
        outIdxCoord[elem + 1] = numCoord;
    }

    for (Index elem = 0; elem < numElem; ++elem) {
        outIdxPoly[elem + 1] += outIdxPoly[elem];
        outIdxCorner[elem + 1] += outIdxCorner[elem];
        outIdxCoord[elem + 1] += outIdxCoord[elem];
    }
    Index numPoly = outIdxPoly[numElem];
    Index numCorner = outIdxCorner[numElem];
    Index numCoord = outIdxCoord[numElem];

    m_outPoly->el().resize(numPoly + 1);
    out_el = m_outPoly->el().data();
    out_el[numPoly] = numCorner;
    m_outPoly->cl().resize(numCorner);
    out_cl = m_outPoly->cl().data();

    m_outPoly->setSize(numCoord);
    out_x = m_outPoly->x().data();
    out_y = m_outPoly->y().data();
    out_z = m_outPoly->z().data();

    if (m_outData) {
        m_outData->x().resize(numCoord);
        out_d = m_outData->x().data();
    }
}


/**
 * @brief Process incoming data.
 *
 * @return true, if everything went well.
 */
bool PlaneClip::process()
{
    processCoordinates();
    Index numCoordsIn = m_outCoords->getNumCoords();

    if (m_tri) {
        const Index numElem = haveCornerList ? m_tri->getNumCorners() / 3 : m_tri->getNumCoords() / 3;
        std::vector<Index> outIdxCorner(numElem + 1), outIdxCoord(numElem + 1);

        prepareTriangles(outIdxCorner, outIdxCoord, numCoordsIn, numElem);
        /* scheduleProcess(false, numElem, outIdxCorner, outIdxCoord); */

#pragma omp parallel for schedule(dynamic)
        for (ssize_t elem = 0; elem < ssize_t(numElem); ++elem)
            processTriangle(elem, outIdxCorner[elem], outIdxCoord[elem], false);

        //std::cerr << "CuttingSurface: << " << m_outData->x().size() << " vert, " << m_outData->x().size() << " data elements" << std::endl;
    } else if (m_poly) {
        const Index numElem = m_poly->getNumElements();
        std::vector<Index> outIdxPoly(numElem + 1), outIdxCorner(numElem + 1), outIdxCoord(numElem + 1);
        outIdxPoly[0] = 0;
        outIdxCorner[0] = 0;
        outIdxCoord[0] = numCoordsIn;

        preparePolygons(outIdxPoly, outIdxCorner, outIdxCoord, numCoordsIn, numElem);
        /* scheduleProcess(false, numElem, outIdxPoly, outIdxCorner, outIdxCoord); */

#pragma omp parallel for schedule(dynamic)
        for (ssize_t elem = 0; elem < ssize_t(numElem); ++elem)
            processPolygon(elem, outIdxPoly[elem], outIdxCorner[elem], outIdxCoord[elem], false);

        //std::cerr << "CuttingSurface: << " << m_outData->x().size() << " vert, " << m_outData->x().size() << " data elements" << std::endl;
    }

    return true;
}

/**
 * @brief Get result.
 *
 * @return return m_outCoords.
 */
Object::ptr PlaneClip::result()
{
    return m_outCoords;
}

void PlaneClip::processCoordinates()
{
    const Index nCoord = m_coord->getNumCoords();
    m_vertexMap.resize(nCoord);
    auto vertexMap = m_vertexMap.data();
#pragma omp parallel for
    for (ssize_t i = 0; i < ssize_t(nCoord); ++i) {
        const Vector3 p(x[i], y[i], z[i]);
        vertexMap[i] = m_decider(i) > 0 ? 1 : 0;
    }

    if (haveCornerList) {
        Index numIn = 0;
        for (Index i = 0; i < nCoord; ++i) {
            if (vertexMap[i]) {
                numIn += vertexMap[i];
                vertexMap[i] = numIn;
            }
        }

        m_outCoords->setSize(numIn);
        out_x = m_outCoords->x().data();
        out_y = m_outCoords->y().data();
        out_z = m_outCoords->z().data();
#pragma omp parallel for schedule(dynamic)
        for (ssize_t i = 0; i < ssize_t(nCoord); ++i) {
            Index idx = vertexMap[i];
            assert(idx >= 0);
            if (idx > 0) {
                --idx;
                assert(idx < numIn);
                out_x[idx] = x[i];
                out_y[idx] = y[i];
                out_z[idx] = z[i];
            }
        }
    }
}

/**
 * @brief Split the edges between two corners specified by given index.
 *
 * @param i First corner.
 * @param j Secont corner.
 *
 * @return Interpolated vector3 object between corners.
 */
Vector3 PlaneClip::splitEdge(Index i, Index j)
{
    Scalar a = m_decider(i);
    Scalar b = m_decider(j);
    const Scalar t = interpolation_weight<Scalar>(a, b, Scalar(0));
    assert(a * b <= 0);
    Vector3 p1(x[i], y[i], z[i]);
    Vector3 p2(x[j], y[j], z[j]);
    return lerp(p1, p2, t);
}

/**
 * @brief Makes sure that all the vertices belonging to a triangle are emitted in order.
 *
 * @param vertexMap Pointer to map from vertex indices in the incoming object to vertex indices in the outgoing object.
 * @param start Start index for this triangle.
 * @param cornerIn Reference to number of corners inside triangle.
 * @param cornerOut Reference to number of corners outside triangle.
 * @param numIn Reference to number of vertices inside triangle.
 */
void PlaneClip::prepareToEmitInOrder(const Index *vertexMap, const Index &start, Index &cornerIn, Index &cornerOut,
                                     Index &numIn)
{
    for (Index i = 0; i < 3; ++i) {
        const Index corner = start + i;
        const Index vind = haveCornerList ? cl[corner] : corner;
        const bool in = vertexMap[vind] > 0;
        if (in) {
            cornerIn = i;
            ++numIn;
        } else {
            cornerOut = i;
        }
    }
}

/**
 * @brief Initialize the prexisting corner.
 *
 * @param idx Current index.
 *
 * @return The prexisting corner from connect list if the cornerlist is available, else simply return the index.
 */
auto PlaneClip::initPreExistCorner(const Index &idx)
{
    return haveCornerList ? cl[idx] : idx;
}

/**
 * @brief Initialize the prexisting corner and check afterwards if it fullfills the logical bool operation given.
 *
 * @param op Logical bool function.
 * @param idx Current index.
 *
 * @return The prexisting corner from connect list if the cornerlist is available and the logical operation is correct, else simply return the index.
 */
auto PlaneClip::initPreExistCornerAndCheck(LogicalOperation op, const Index &idx)
{
    auto pre = initPreExistCorner(idx);
    assert(op(pre));
    return pre;
}

/**
 * @brief Copy given Vector3 values to out_x, out_y and out_z according to given index list for output (outIdxList) and for given vector index list (vecIdxList). If output index list is not given the given index idx will be used for all outputs. If vector list is not given the default values will be {0,1,2}.
 *
 * @param vec Vector3 object to copy.
 * @param idx Current index.
 * @param outIdxList Index list for output coords (optional).
 * @param vecIdxList Index list for Vector3 coords (optional).
 */
void PlaneClip::copyVec3ToOutCoords(const vistle::Vector3 &vec, const Index &idx,
                                    const std::array<Index, 3> &outIdxList, const std::array<Index, 3> &vecIdxList)
{
    if (outIdxList[0] != InvalidIndex) {
        out_x[outIdxList[0]] = vec[vecIdxList[0]];
        out_y[outIdxList[1]] = vec[vecIdxList[1]];
        out_z[outIdxList[2]] = vec[vecIdxList[2]];
    } else {
        out_x[idx] = vec[vecIdxList[0]];
        out_y[idx] = vec[vecIdxList[1]];
        out_z[idx] = vec[vecIdxList[2]];
    }
}

/**
 * @brief Copy x, y and z values from index in to out_x, out_y and out_z at index out.
 *
 * @param out Output index.
 * @param in Input index.
 */
void PlaneClip::copyScalarToOutCoords(const Index &out, const Index &in)
{
    out_x[out] = x[in];
    out_y[out] = y[in];
    out_z[out] = z[in];
}

/**
 * @brief Copy given index to output connect list.
 *
 * @param out Index for output.
 * @param idx Value to copy.
 */
void PlaneClip::copyIdxToOutConnList(const Index &out, const Index &idx)
{
    out_cl[out] = idx;
}

/**
 * @brief Copy given indeces to output connect list by starting with index out and incrementing this by one with each iteration.
 *
 * @param out Start index for ouput.
 * @param vecIdx Values to assign.
 *
 * @return Last index of output assignment.
 */
auto PlaneClip::copyIndecesToOutConnList(const Index &out, const std::vector<Index> &vecIdx)
{
    Index n{0};
    std::for_each(vecIdx.begin(), vecIdx.end(), [&](const auto &idx) {
        copyIdxToOutConnList(out + n, idx);
        ++n;
    });
    return n;
}

/**
 * @brief Copy given indeces to output connect list by starting with index out and incrementing this by one with each iteration and check logical operation on result (useful for debugging).
 *
 * @param op Boolsche logical operation.
 * @param out Start index for ouput.
 * @param vecIdx Values to assign.
 */
void PlaneClip::copyIndecesToOutConnListAndCheck(LogicalOperation op, const Index &out,
                                                 const std::vector<Index> &vecIdx)
{
    assert(op(copyIndecesToOutConnList(out, vecIdx)));
}

/**
 * @brief Split the edges and copy given Vector3 values to out_x, out_y and out_z.
 *
 * @param idx Current index.
 * @param in Corner in visible area.
 * @param out Corner out of visible area.
 */
void PlaneClip::copySplitEdgeResultToOutCoords(const Index &idx, const Index &in, const Index &out)
{
    const Vector3 vec = splitEdge(in, out);
    copyVec3ToOutCoords(vec, idx);
}

/**
 * @brief Iterate over arguments given and copy each Vector3 to out_x, out_y and out_z while incrementing given index.
 *
 * @tparam VistleVec3Args Vector3 arguments.
 * @param idx Current index.
 * @param ...vecs Parameter pack for Vector3 arguments.
 */
template<typename... VistleVec3Args>
void PlaneClip::iterCopyOfVec3ToOutCoords(Index &idx, VistleVec3Args &&...vecs)
{
    auto copyWrapper = [&](const Vector3 &vec) {
        copyVec3ToOutCoords(vec, idx);
        ++idx;
    };
    (copyWrapper(vecs), ...);
}

/**
 * @brief Inserts the element and all vertices, if all vertices in the element are on the right side of the cutting plane.
 *
 * @param numVertsOnly Use only the number of vertices to set indeces of corner and coordinates.
 * @param vertexMap Pointer to map from vertex indices in the incoming object to vertex indices in the outgoing object.
 * @param start Start index for this triangle in vertexMap.
 * @param outIdxCorner Index of first corner outside.
 * @param outIdxCoord Index of first coordinates outside.
 */
void PlaneClip::insertTriElemNextToCutPlane(bool numVertsOnly, const Index *vertexMap, const Index &start,
                                            Index &outIdxCorner, Index &outIdxCoord)
{
    if (numVertsOnly) {
        outIdxCorner = haveCornerList ? 3 : 0;
        outIdxCoord = haveCornerList ? 0 : 3;
        return;
    }

    if (haveCornerList)
        fillConnListIfElemVisible(start, 3, outIdxCorner, Geometry::Triangle);
    else {
        for (Index i = 0; i < 3; ++i) {
            const Index in = start + i;
            const Index out = outIdxCoord + i;
            copyScalarToOutCoords(out, in);
        }
    }
}

/**
 * @brief Inserts the element and all vertices, if all vertices in the element are on the right side of the cutting plane. Used if not all corners of a triangle are in the visible area.
 *
 * @param numVertsOnly Use only the number of vertices to set indeces of corner and coordinates.
 * @param vertexMap Pointer to map from vertex indices in the incoming object to vertex indices in the outgoing object.
 * @param numIn Corners inside visible area.
 * @param cornerIn For the case where we have to split edges, new triangles might be created => corner inside.
 * @param cornerOut For the case where we have to split edges, new triangles might be created => corner outside.
 * @param start Start index for this triangle in vertexMap.
 * @param outIdxCorner Index ref to corner index to assign new number of corners.
 * @param outIdxCoord Index ref to coordinate index to assign new number of coords.
 */
void PlaneClip::insertTriElemPartNextToCutPlane(bool numVertsOnly, const Index *vertexMap, const Index &numIn,
                                                const Index &start, const Index &cornerIn, const Index &cornerOut,
                                                Index &outIdxCorner, Index &outIdxCoord)
{
    const Index totalCorner = numIn == 1 ? 3 : 9;
    const Index newCoord = numIn == 1 ? 2 : 3;
    auto inPredicate = [&vertexMap](const auto &idx) -> bool {
        return vertexMap[idx] > 0;
    };
    auto outPredicate = [&vertexMap](const auto &idx) -> bool {
        return vertexMap[idx] == 0;
    };
    // for debugging
    /* auto totalCornerPredicate = [&totalCorner](const Index &idx) -> bool { */
    /*     return idx == totalCorner; */
    /* }; */

    if (numVertsOnly) {
        outIdxCorner = haveCornerList ? totalCorner : 0;
        outIdxCoord = haveCornerList ? newCoord : totalCorner;
        return;
    }

    if (numIn == 1) {
        // in0 is the only pre-existing corner inside
        const Index in0 = initPreExistCornerAndCheck(inPredicate, start + cornerIn);
        const Index out0 = initPreExistCornerAndCheck(outPredicate, start + (cornerIn + 1) % 3);
        const Index out1 = initPreExistCornerAndCheck(outPredicate, start + (cornerIn + 2) % 3);

        copySplitEdgeResultToOutCoords(outIdxCoord, in0, out0);
        copySplitEdgeResultToOutCoords(outIdxCoord + 1, in0, out1);

        if (haveCornerList)
            //debug
            /* copyIndecesToOutConnListAndCheck(totalCornerPredicate, outIdxCorner, */
            copyIndecesToOutConnList(outIdxCorner,
                                     std::vector<Index>{vertexMap[in0] - 1, outIdxCoord, outIdxCoord + 1});
        else
            copyScalarToOutCoords(outIdxCoord + 2, in0);
    } else if (numIn == 2) {
        auto out0 = initPreExistCornerAndCheck(outPredicate, start + cornerOut);
        auto in0 = initPreExistCornerAndCheck(inPredicate, start + (cornerOut + 2) % 3);
        auto in1 = initPreExistCornerAndCheck(inPredicate, start + (cornerOut + 1) % 3);

        copySplitEdgeResultToOutCoords(outIdxCoord, in0, out0);
        copySplitEdgeResultToOutCoords(outIdxCoord + 1, in1, out0);
        const Vector3 vin0(x[in0], y[in0], z[in0]);
        const Vector3 vin1(x[in1], y[in1], z[in1]);
        const Vector3 v2 = (vin0 + vin1) * Scalar(0.5);

        copyVec3ToOutCoords(v2, outIdxCoord + 2);

        if (haveCornerList)
            //debug
            /* copyIndecesToOutConnListAndCheck(totalCornerPredicate, outIdxCorner, */
            copyIndecesToOutConnList(outIdxCorner,
                                     std::vector<Index>{vertexMap[in0] - 1, outIdxCoord, outIdxCoord + 2,
                                                        outIdxCoord + 2, outIdxCoord, outIdxCoord + 1, outIdxCoord + 2,
                                                        outIdxCoord + 1, vertexMap[in1] - 1});
        else {
            const Vector3 v0 = splitEdge(in0, out0);
            const Vector3 v1 = splitEdge(in1, out0);

            Index out = outIdxCoord;
            copyScalarToOutCoords(out++, in0);
            iterCopyOfVec3ToOutCoords(out, v0, v2, v2, v0, v1, v2, v1);
            copyScalarToOutCoords(out, in1);
        }
    }
}

/**
 * @brief Process a triangle for inserting if its partially or in a whole in the visible area.
 *
 * @param element Index of triangle.
 * @param outIdxCorner Index ref to corner index to assign new number of corners.
 * @param outIdxCoord Index ref to coordinate index to assign new number of coords.
 * @param numVertsOnly process triangle by only using vertices.
 */
void PlaneClip::processTriangle(const Index element, Index &outIdxCorner, Index &outIdxCoord, bool numVertsOnly)
{
    const Index start = element * 3;
    const auto vertexMap = m_vertexMap.data();

    Index numIn = 0;
    Index cornerIn = 0, cornerOut = 0; // for the case where we have to split edges, new triangles might be created
    prepareToEmitInOrder(vertexMap, start, cornerIn, cornerOut, numIn);

    if (numIn == 0) {
        if (numVertsOnly) {
            outIdxCorner = 0;
            outIdxCoord = 0;
        }
        return;
    }

    if (numIn == 3)
        insertTriElemNextToCutPlane(numVertsOnly, vertexMap, start, outIdxCorner, outIdxCoord);
    else if (numIn > 0)
        insertTriElemPartNextToCutPlane(numVertsOnly, vertexMap, numIn, start, cornerIn, cornerOut, outIdxCorner,
                                        outIdxCoord);
}

/**
 * @brief HELPER: iterate operation of for loops in  fillConnListIfElemVisible.
 *
 * @param vertexMap vertex specific data.
 * @param corner current corner.
 * @param outIdxCorner
 * @param it current index.
 */
void PlaneClip::helper_fillConnListIfElemVisible(const Index *vertexMap, const Index &corner, const Index &outIdxCorner,
                                                 const Index &it)
{
    const Index vid = cl[corner];
    const Index outID = vertexMap[vid] - 1;
    out_cl[outIdxCorner + it] = outID;
}

/**
 * @brief Fill connection list in case all elemets and vertices are on the right side (check not included).
 *
 * @param start Start index of corner of polygon in cornerlist.
 * @param end End index of corner of polygon in cornerlist.
 * @param outIdxCorner
 */
void PlaneClip::fillConnListIfElemVisible(const Index &start, const Index &end, const Index &outIdxCorner,
                                          const Geometry &geo)
{
    const auto vertexMap = m_vertexMap.data();
    switch (geo) {
    case Geometry::Triangle:
        for (Index i = 0; i < end; ++i) {
            const Index corner = start + i;
            helper_fillConnListIfElemVisible(vertexMap, corner, outIdxCorner, i);
        }
        break;
    case Geometry::Polygon: //default
        Index i = 0;
        for (Index corner = start; corner < end; ++corner, ++i)
            helper_fillConnListIfElemVisible(vertexMap, corner, outIdxCorner, i);
        break;
    }
}

/**
 * @brief If all vertices in the element are on the right side of the cutting plane, insert the element and all vertices.
 *
 * @param numVertsOnly Use only the number of vertices to set indeces of corner, element number of polygons and coordinates.
 * @param vertexMap Pointer to map from vertex indices in the incoming object to vertex indices in the outgoing object.
 * @param numIn Corners inside visible area.
 * @param start Start index for this triangle in vertexMap.
 * @param outIdxPoly Index ref to polygon element to assign new number of polygons.
 * @param outIdxCorner Index ref to corner index to assign new number of corners.
 * @param outIdxCoord Index ref to coordinate index to assign new number of coords.
 */
void PlaneClip::insertPolyElemNextToPlane(bool numVertsOnly, const Index &numIn, const Index &start, const Index &end,
                                          Index &outIdxPoly, Index &outIdxCorner, Index &outIdxCoord)
{
    if (numVertsOnly) {
        outIdxPoly = 1;
        outIdxCorner = numIn;
        outIdxCoord = 0;
        return;
    }

    out_el[outIdxPoly] = outIdxCorner;
    fillConnListIfElemVisible(start, end, outIdxCorner);
}

/**
 * @brief If not all of the vertices of an element are on the same side of the cutting plane:
 *     - insert vertices that are on the right side of the plane
 *     - omit vertices that are on the wrong side of the plane
 *     - if the vertex before the processed vertex is on the
 *       other side of the plane: insert the intersection point
 *       between the line formed by the two vertices and the
 *       plane
 *
 * @param numVertsOnly Use only the number of vertices to set indeces of corner, element number of polygons and coordinates.
 * @param numCreate
 * @param numCorner
 * @param numIn Corners inside visible area.
 * @param start Start index for this triangle in vertexMap.
 * @param outIdxPoly Index ref to polygon element to assign new number of polygons.
 * @param outIdxCorner Index ref to corner index to assign new number of corners.
 * @param outIdxCoord Index ref to coordinate index to assign new number of coords.
 */
void PlaneClip::insertPolyElemPartNextToPlane(bool numVertsOnly, const Index &numCreate, const Index &numCorner,
                                              const Index &numIn, const Index &start, const Index &end,
                                              Index &outIdxPoly, Index &outIdxCorner, Index &outIdxCoord)
{
    const auto vertexMap = m_vertexMap.data();
    assert(numCreate % 2 == 0);
    assert(numCreate == 2); // we only handle convex polygons
    if (numVertsOnly) {
        outIdxPoly = 1;
        outIdxCorner = numIn + numCreate;
        outIdxCoord = numCreate;
        return;
    }

    out_el[outIdxPoly] = outIdxCorner;

    Index n = 0;
    Index prevIdx = cl[start + numCorner - 1];
    Index numCreated = 0;
    bool prevIn = vertexMap[prevIdx] > 0;
    for (Index i = 0; i < numCorner; ++i) {
        const Index corner = start + i;
        const Index idx = cl[corner];
        Index outID = vertexMap[idx];
        const bool in = outID > 0;
        --outID;
        if (in != prevIn) {
            const Index newId = outIdxCoord + numCreated;
            ++numCreated;

            copySplitEdgeResultToOutCoords(newId, idx, prevIdx);

            out_cl[outIdxCorner + n] = newId;
            ++n;
        }
        if (in) {
            out_cl[outIdxCorner + n] = outID;
            ++n;
        }
        prevIdx = idx;
        prevIn = in;
    }
    assert(numCreated == numCreate);
    assert(n == numIn + numCreate);
}

/**
 * @brief Process a polygon for inserting if its partially or in a whole in the visible area.
 *
 *
 * @param element Index of triangle.
 * @param outIdxPoly Index ref to polygon element to assign new number of polygons.
 * @param outIdxCorner Index ref to corner index to assign new number of corners.
 * @param outIdxCoord Index ref to coordinate index to assign new number of coords.
 * @param numVertsOnly process triangle by only using vertices.
 */
void PlaneClip::processPolygon(const Index element, Index &outIdxPoly, Index &outIdxCorner, Index &outIdxCoord,
                               bool numVertsOnly)
{
    const auto vertexMap = m_vertexMap.data();
    const Index start = el[element];
    Index end;
    if (element != m_poly->getNumElements() - 1)
        end = el[element + 1];
    else
        end = m_poly->getNumCorners();
    const Index nCorner = end - start;

    Index numIn = 0, numCreate = 0;
    if (nCorner > 0) {
        bool prevIn = vertexMap[cl[end - 1]] > 0;
        for (Index corner = start; corner < end; ++corner) {
            const Index vind = cl[corner];
            const bool in = vertexMap[vind] > 0;
            if (in) {
                if (!prevIn) {
                    ++numCreate;
                }
                ++numIn;
                prevIn = true;
            } else {
                if (prevIn)
                    ++numCreate;
                prevIn = false;
            }
        }
    }

    if (numIn == 0) {
        if (numVertsOnly) {
            outIdxPoly = 0;
            outIdxCorner = 0;
            outIdxCoord = 0;
        }
        return;
    }

    if (numIn == nCorner)
        insertPolyElemNextToPlane(numVertsOnly, numIn, start, end, outIdxPoly, outIdxCorner, outIdxCoord);
    else if (numIn > 0)
        insertPolyElemPartNextToPlane(numVertsOnly, numCreate, nCorner, numIn, start, end, outIdxPoly, outIdxCorner,
                                      outIdxCoord);
}
