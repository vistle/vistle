#include "PlaneClip.h"
#include "vistle/core/index.h"
#include "vistle/core/vector.h"
#include <algorithm>

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
        cl = &grid->cl()[0];
    }
    x = &grid->x()[0];
    y = &grid->y()[0];
    z = &grid->z()[0];

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
    el = &grid->el()[0];
    cl = &grid->cl()[0];
    x = &grid->x()[0];
    y = &grid->y()[0];
    z = &grid->z()[0];

    m_outPoly = Polygons::ptr(new Polygons(Object::Initialized));
    m_outPoly->setMeta(grid->meta());
    m_outCoords = m_outPoly;
}

void PlaneClip::addData(Object::const_ptr obj)
{
    m_data.push_back(obj);
    auto data = Vec<Scalar, 1>::as(obj);
    if (data) {
        d = &data->x()[0];

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

Vector3 PlaneClip::splitEdge(Index i, Index j)
{
    Scalar a = m_decider(i);
    Scalar b = m_decider(j);
    const Scalar t = tinterp(0, a, b);
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
 * @brief Inserts the element and all vertices, if all vertices in the element are on the right side of the cutting plane.
 *
 * @param numVertsOnly Use only the number of vertices to set indeces of corner and coordinates.
 * @param vertexMap Pointer to map from vertex indices in the incoming object to vertex indices in the outgoing object.
 * @param start Start index for this triangle in vertexMap.
 * @paparam outIdxCorner Index reference to first corner outside.
 * @param outIdxCoord Index reference to first coordinate outside.
 */
void PlaneClip::insertElemNextToCutPlane(bool numVertsOnly, const Index *vertexMap, const Index &start,
                                         Index &outIdxCorner, Index &outIdxCoord)
{
    if (numVertsOnly) {
        outIdxCorner = haveCornerList ? 3 : 0;
        outIdxCoord = haveCornerList ? 0 : 3;
        return;
    }

    if (haveCornerList) {
        for (Index i = 0; i < 3; ++i) {
            const Index corner = start + i;
            const Index vid = cl[corner];
            const Index outID = vertexMap[vid] - 1;
            out_cl[outIdxCorner + i] = outID;
        }
    } else {
        for (Index i = 0; i < 3; ++i) {
            const Index in = start + i;
            const Index out = outIdxCoord + i;
            out_x[out] = x[in];
            out_y[out] = y[in];
            out_z[out] = z[in];
        }
    }
}

auto PlaneClip::initPreExisting(const Index &idx)
{
    return haveCornerList ? cl[idx] : idx;
}

auto PlaneClip::initPreExistingAndCheck(LogicalOperation op, const Index &idx)
{
    auto pre = initPreExisting(idx);
    assert(op(pre));
    return pre;
}

void PlaneClip::copySplitEdgeResultToOutCoords(const Index &curIdx, const Index &in, const Index &out)
{
    const Vector3 vec = splitEdge(in, out);
    out_x[curIdx] = vec[0];
    out_y[curIdx] = vec[1];
    out_z[curIdx] = vec[2];
}

void PlaneClip::insertElemNextToCutPlane(bool numVertsOnly, const Index *vertexMap, const Index &numIn,
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

    if (numVertsOnly) {
        outIdxCorner = haveCornerList ? totalCorner : 0;
        outIdxCoord = haveCornerList ? newCoord : totalCorner;
        return;
    }

    if (numIn == 1) {
        // in0 is the only pre-existing corner inside
        const Index in0 = initPreExistingAndCheck(inPredicate, start + cornerIn);
        const Index out0 = initPreExistingAndCheck(outPredicate, start + (cornerIn + 1) % 3);
        const Index out1 = initPreExistingAndCheck(outPredicate, start + (cornerIn + 2) % 3);

        copySplitEdgeResultToOutCoords(outIdxCoord, in0, out0);
        copySplitEdgeResultToOutCoords(outIdxCoord + 1, in0, out1);

        if (haveCornerList) {
            Index n = 0;
            out_cl[outIdxCorner + n] = vertexMap[in0] - 1;
            ++n;
            out_cl[outIdxCorner + n] = outIdxCoord;
            ++n;
            out_cl[outIdxCorner + n] = outIdxCoord + 1;
            ++n;
            assert(n == totalCorner);
        } else {
            out_x[outIdxCoord + 2] = x[in0];
            out_y[outIdxCoord + 2] = y[in0];
            out_z[outIdxCoord + 2] = z[in0];
        }
    } else if (numIn == 2) {
        auto out0 = initPreExistingAndCheck(outPredicate, start + cornerOut);
        auto in0 = initPreExistingAndCheck(inPredicate, start + (cornerOut + 2) % 3);
        copySplitEdgeResultToOutCoords(outIdxCoord, in0, out0);

        auto in1 = initPreExistingAndCheck(inPredicate, start + (cornerOut + 1) % 3);
        copySplitEdgeResultToOutCoords(outIdxCoord + 1, in1, out0);
        const Vector3 vin0(x[in0], y[in0], z[in0]);
        const Vector3 vin1(x[in1], y[in1], z[in1]);
        const Vector3 v2 = (vin0 + vin1) * Scalar(0.5);

        out_x[outIdxCoord + 2] = v2[0];
        out_y[outIdxCoord + 2] = v2[1];
        out_z[outIdxCoord + 2] = v2[2];

        if (haveCornerList) {
            Index n = 0;
            out_cl[outIdxCorner + n] = vertexMap[in0] - 1;
            ++n;
            out_cl[outIdxCorner + n] = outIdxCoord;
            ++n;
            out_cl[outIdxCorner + n] = outIdxCoord + 2;
            ++n;

            out_cl[outIdxCorner + n] = outIdxCoord + 2;
            ++n;
            out_cl[outIdxCorner + n] = outIdxCoord;
            ++n;
            out_cl[outIdxCorner + n] = outIdxCoord + 1;
            ++n;

            out_cl[outIdxCorner + n] = outIdxCoord + 2;
            ++n;
            out_cl[outIdxCorner + n] = outIdxCoord + 1;
            ++n;
            out_cl[outIdxCorner + n] = vertexMap[in1] - 1;
            ++n;
            assert(n == totalCorner);
        } else {
            const Vector3 v0 = splitEdge(in0, out0);
            const Vector3 v1 = splitEdge(in1, out0);

            Index out = outIdxCoord;
            out_x[out] = x[in0];
            out_y[out] = y[in0];
            out_z[out] = z[in0];
            ++out;
            out_x[out] = v0[0];
            out_y[out] = v0[1];
            out_z[out] = v0[2];
            ++out;
            out_x[out] = v2[0];
            out_y[out] = v2[1];
            out_z[out] = v2[2];
            ++out;

            out_x[out] = v2[0];
            out_y[out] = v2[1];
            out_z[out] = v2[2];
            ++out;
            out_x[out] = v0[0];
            out_y[out] = v0[1];
            out_z[out] = v0[2];
            ++out;
            out_x[out] = v1[0];
            out_y[out] = v1[1];
            out_z[out] = v1[2];
            ++out;

            out_x[out] = v2[0];
            out_y[out] = v2[1];
            out_z[out] = v2[2];
            ++out;
            out_x[out] = v1[0];
            out_y[out] = v1[1];
            out_z[out] = v1[2];
            ++out;
            out_x[out] = x[in1];
            out_y[out] = y[in1];
            out_z[out] = z[in1];
            ++out;
        }
    }
}

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
        insertElemNextToCutPlane(numVertsOnly, vertexMap, start, outIdxCorner, outIdxCoord);
    else if (numIn > 0)
        insertElemNextToCutPlane(numVertsOnly, vertexMap, numIn, start, cornerIn, cornerOut, outIdxCorner, outIdxCoord);
}

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

    if (numIn == nCorner) {
        // if all vertices in the element are on the right side
        // of the cutting plane, insert the element and all vertices

        if (numVertsOnly) {
            outIdxPoly = 1;
            outIdxCorner = numIn;
            outIdxCoord = 0;
            return;
        }

        out_el[outIdxPoly] = outIdxCorner;

        Index i = 0;
        for (Index corner = start; corner < end; ++corner) {
            const Index vid = cl[corner];
            const Index outID = vertexMap[vid] - 1;
            out_cl[outIdxCorner + i] = outID;
            ++i;
        }

    } else if (numIn > 0) {
        // if not all of the vertices of an element are on the same
        // side of the cutting plane:
        //   - insert vertices that are on the right side of the plane
        //   - omit vertices that are on the wrong side of the plane
        //   - if the vertex before the processed vertex is on the
        //     other side of the plane: insert the intersection point
        //     between the line formed by the two vertices and the
        //     plane

        assert(numCreate % 2 == 0);
        assert(numCreate == 2); // we only handle convex polygons
        if (numVertsOnly) {
            outIdxPoly = numCreate / 2;
            outIdxPoly = 1;
            outIdxCorner = numIn + numCreate;
            outIdxCoord = numCreate;
            return;
        }

        out_el[outIdxPoly] = outIdxCorner;

        Index n = 0;
        Index prevIdx = cl[start + nCorner - 1];
        Index numCreated = 0;
        bool prevIn = vertexMap[prevIdx] > 0;
        for (Index i = 0; i < nCorner; ++i) {
            const Index corner = start + i;
            const Index idx = cl[corner];
            Index outID = vertexMap[idx];
            const bool in = outID > 0;
            --outID;
            if (in != prevIn) {
                const Index newId = outIdxCoord + numCreated;
                ++numCreated;

                const Vector3 v = splitEdge(idx, prevIdx);
                out_x[newId] = v[0];
                out_y[newId] = v[1];
                out_z[newId] = v[2];

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
}
