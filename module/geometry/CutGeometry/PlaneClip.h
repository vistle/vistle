#ifndef VISTLE_CUTGEOMETRY_PLANECLIP_H
#define VISTLE_CUTGEOMETRY_PLANECLIP_H

#include <functional>
#include <tuple>
#include <utility>
#include <vistle/core/triangles.h>
#include <vistle/core/polygons.h>
#include "../../map/IsoSurface/IsoDataFunctor.h"
#include "vistle/core/index.h"
#include "vistle/core/vector.h"

using namespace vistle;


class PlaneClip {
public:
    PlaneClip(Triangles::const_ptr grid, IsoDataFunctor decider);
    PlaneClip(Polygons::const_ptr grid, IsoDataFunctor decider);

    template<typename F, typename... Args>
    auto for_each_arg(ssize_t elem, F f, Args &&...args)
    {
        return std::make_tuple(f(elem, args)...);
    }

    typedef std::vector<Index> IdxVec;
    template<typename... idxVecs>
    void scheduleProcess(bool numVertsOnly, Index numElem, idxVecs &&...vecs)
    {
        auto accessElem = [](const ssize_t &e, const auto &vec) {
            return vec[e];
        };
#pragma omp parallel for schedule(dynamic)
        for (ssize_t elem = 0; elem < ssize_t(numElem); ++elem) {
            auto t = std::make_tuple(numVertsOnly, elem);
            auto forward = std::tuple_cat(t, for_each_arg(elem, accessElem, vecs...));
            std::apply([this](auto &&...args) { processParallel(args...); }, forward);
        }
    }

    void addData(Object::const_ptr obj);
    bool process();
    Object::ptr result();

private:
    typedef std::function<bool(const Index &)> LogicalOperation;
    enum Geometry { Triangle, Polygon };

    Vector3 splitEdge(Index i, Index j);
    void processCoordinates();
    void helper_fillConnListIfElemVisible(const Index *vertexMap, const Index &corner, const Index &outIdxCorner,
                                          const Index &it);
    void fillConnListIfElemVisible(const Index &start, const Index &end, const Index &outIdxCorner,
                                   const Geometry &geo = Geometry::Polygon);

    //triangles
    auto initPreExistCorner(const Index &idx);
    auto initPreExistCornerAndCheck(LogicalOperation op, const Index &idx);
    void copySplitEdgeResultToOutCoords(const Index &idx, const Index &in, const Index &out);
    void prepareTriangles(std::vector<Index> &outIdxCorner, std::vector<Index> &outIdxCoord, const Index &numCoordsIn,
                          const Index &numElem);
    void processTriangle(const Index element, Index &outIdxCorner, Index &outIdxCoord, bool numVertsOnly);
    void prepareToEmitInOrder(const Index *vertexMap, const Index &start, Index &cornerIn, Index &cornerOut,
                              Index &numIn);
    void insertTriElemNextToCutPlane(bool numVertsOnly, const Index *vertexMap, const Index &start, Index &outIdxCorner,
                                     Index &outIdxCoord);
    void insertTriElemPartNextToCutPlane(bool numVertsOnly, const Index *vertexMap, const Index &numIn,
                                         const Index &start, const Index &cornerIn, const Index &cornerOut,
                                         Index &outIdxCorner, Index &outIdxCoord);
    void copyVec3ToOutCoords(const vistle::Vector3 &vec, const Index &idx,
                             const std::array<Index, 3> &outIdxList = {InvalidIndex, InvalidIndex, InvalidIndex},
                             const std::array<Index, 3> &vecIdxList = {0, 1, 2});
    void copyScalarToOutCoords(const Index &out, const Index &in);
    void copyIdxToOutConnList(const Index &out, const Index &idx);
    auto copyIndecesToOutConnList(const Index &out, const std::vector<Index> &vecIdx);
    void copyIndecesToOutConnListAndCheck(LogicalOperation op, const Index &out, const std::vector<Index> &vecIdx);
    template<typename... VistleVec3Args>
    void iterCopyOfVec3ToOutCoords(Index &idx, VistleVec3Args &&...vecs);

    //polygons
    void preparePolygons(std::vector<Index> &outIdxPoly, std::vector<Index> &outIdxCorner,
                         std::vector<Index> &outIdxCoord, const Index &numCoordsIn, const Index &numElem);
    void processPolygon(const Index element, Index &outIdxPoly, Index &outIdxCorner, Index &outIdxCoord,
                        bool numVertsOnly);
    void reassignPolygonData(const Index &poly, const Index &corner, const Index &coord);
    void insertPolyElemNextToPlane(bool numVertsOnly, const Index &numIn, const Index &start, const Index &end,
                                   Index &outIdxPoly, Index &outIdxCorner, Index &outIdxCoord);
    void insertPolyElemPartNextToPlane(bool numVertsOnly, const Index &numCreate, const Index &numCorner,
                                       const Index &numIn, const Index &start, const Index &end, Index &outIdxPoly,
                                       Index &outIdxCorner, Index &outIdxCoord);

    void processParallel(bool numVertsOnly, const Index element, Index &outIdxCorner, Index &outIdxCoord)
    {
        processTriangle(element, outIdxCoord, outIdxCoord, numVertsOnly);
    }

    void processParallel(bool numVertsOnly, const Index element, Index &outIdxPoly, Index &outIdxCorner,
                         Index &outIdxCoord)
    {
        processPolygon(element, outIdxCoord, outIdxCoord, outIdxCoord, numVertsOnly);
    }

    /* #define PROCESS_PARALLEL_IMPL(FUNC_NAME,...) \ */
    /* template<typename... Args> \ */
    /* void processParallel(Args&&... args) \ */
    /* { \ */
    /*     FUNC_NAME(std::forward<Args>(args)...); \ */
    /* } \ */

    /* PROCESS_PARALLEL_IMPL(processTriangle); */
    /* PROCESS_PARALLEL_IMPL(processPolygon); */

private:
    Coords::const_ptr m_coord;
    Triangles::const_ptr m_tri;
    Polygons::const_ptr m_poly;
    std::vector<Object::const_ptr> m_data;
    IsoDataFunctor m_decider;

    bool haveCornerList; // triangle with trivial index list: every coordinate is used exactly once
    const Index *el;
    const Index *cl;
    const Scalar *x;
    const Scalar *y;
    const Scalar *z;

    const Scalar *d;

    Index *out_cl;
    Index *out_el;
    Scalar *out_x;
    Scalar *out_y;
    Scalar *out_z;
    Scalar *out_d;

    // mapping from vertex indices in the incoming object to
    // vertex indices in the outgoing object
    // - 1 is added to the entry, 0 marks entries that are to be erased
    std::vector<Index> m_vertexMap;

    Coords::ptr m_outCoords;
    Triangles::ptr m_outTri;
    Polygons::ptr m_outPoly;
    Vec<Scalar>::ptr m_outData;
};

#endif
