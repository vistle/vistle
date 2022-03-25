#ifndef PLANECLIP_H
#define PLANECLIP_H

#include <vistle/core/triangles.h>
#include <vistle/core/polygons.h>
#include "../IsoSurface/IsoDataFunctor.h"

using namespace vistle;

class PlaneClip {
public:
    PlaneClip(Triangles::const_ptr grid, IsoDataFunctor decider);
    PlaneClip(Polygons::const_ptr grid, IsoDataFunctor decider);

    void addData(Object::const_ptr obj);
    bool process();
    Object::ptr result();

private:
    Vector3 splitEdge(Index i, Index j);
    void processCoordinates();
    void processTriangle(const Index element, Index &outIdxCorner, Index &outIdxCoord, bool numVertsOnly);
    void processPolygon(const Index element, Index &outIdxPoly, Index &outIdxCorner, Index &outIdxCoord,
                        bool numVertsOnly);

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
