#ifndef PLANECLIP_H
#define PLANECLIP_H

#include <functional>
#include <tuple>
#include <utility>
#include <vistle/core/triangles.h>
#include <vistle/core/polygons.h>
#include "../IsoSurface/IsoDataFunctor.h"
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
    Vector3 splitEdge(Index i, Index j);
    auto initPreExistCorner(const Index &idx);

    typedef std::function<bool(const Index &)> LogicalOperation;
    auto initPreExistCornerAndCheck(LogicalOperation op, const Index &idx);
    void copySplitEdgeResultToOutCoords(const Index &idx, const Index &in, const Index &out);
    void prepareTriangles(std::vector<Index> &outIdxCorner, std::vector<Index> &outIdxCoord, const Index &numCoordsIn,
                          const Index &numElem);
    void preparePolygons(std::vector<Index> &outIdxPoly, std::vector<Index> &outIdxCorner,
                         std::vector<Index> &outIdxCoord, const Index &numCoordsIn, const Index &numElem);
    void processTriangle(const Index element, Index &outIdxCorner, Index &outIdxCoord, bool numVertsOnly);
    void prepareToEmitInOrder(const Index *vertexMap, const Index &start, Index &cornerIn, Index &cornerOut,
                              Index &numIn);
    void processPolygon(const Index element, Index &outIdxPoly, Index &outIdxCorner, Index &outIdxCoord,
                        bool numVertsOnly);
    void processCoordinates();
    void insertElemNextToCutPlane(bool numVertsOnly, const Index *vertexMap, const Index &start, Index &outIdxCorner,
                                  Index &outIdxCoord);
    void insertElemNextToCutPlane(bool numVertsOnly, const Index *vertexMap, const Index &numIn, const Index &start,
                                  const Index &cornerIn, const Index &cornerOut, Index &outIdxCorner,
                                  Index &outIdxCoord);
    void copyVec3ToOutCoords(const vistle::Vector3 &vec, const Index &idx, const std::array<Index, 3> &outIdxList = {},
                             const std::array<Index, 3> &vecIdxList = {0, 1, 2});

    template<typename... VistleVec3Args>
    void iterCopyOfVec3ToOutCoords(Index &idx, VistleVec3Args &&...vecs);

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
