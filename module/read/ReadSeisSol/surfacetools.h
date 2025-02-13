#ifndef VISTLE_READSEISSOL_SURFACETOOLS_H
#define VISTLE_READSEISSOL_SURFACETOOLS_H

#include <vistle/core/celltypes.h>
#include <vistle/core/indexed.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>

#include <iterator>

namespace vistle {
namespace {
template<class T>
struct IndexedVars {
    T numElem;
    T numVertices;
    T numCorners;

    IndexedVars(const T &elem, const T &vert, const T &corners): numElem(elem), numVertices(vert), numCorners(corners)
    {}
};

} // namespace

void addSurfaceFaces(const Polygons::ptr poly, const UnstructuredGrid::ptr &ugrid);

/**
  * DomainSurface-Code for internal use. src: /vistle/module/generald/DomainSurface/DomainSurface.cpp
  */
template<class ContainerIt, class ContainerItType, class T>
Polygons::ptr extractSurface(ContainerIt beginElemList, ContainerIt beginConnectList, ContainerItType beginTypeList,
                             const IndexedVars<T> &vars, const cell::CellType &cellType)
{
    //surface
    Polygons::ptr poly(new Polygons(0, 0, 0));

    //create unstructured grid
    UnstructuredGrid::ptr ugrid(new UnstructuredGrid(vars.numElem, vars.numCorners, vars.numVertices));
    std::copy(beginElemList, beginElemList + vars.numElem, ugrid->el());
    std::copy(beginConnectList, beginConnectList + vars.numElem, ugrid->cl());
    std::copy(beginTypeList, beginTypeList + vars.numElem, ugrid->tl());

    addSurfaceFaces(poly, ugrid, vars);

    if (poly->getNumElements() == 0)
        return Polygons::ptr();

    return poly;
}
} // namespace vistle
#endif
