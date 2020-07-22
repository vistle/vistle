/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <vtkVersion.h>
#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkDataSetAttributes.h>

#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkCharArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkShortArray.h>
#include <vtkUnsignedShortArray.h>
#include <vtkIntArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkIdTypeArray.h>

#include <vtkAlgorithm.h>
#include <vtkInformation.h>
#include <vtkUnstructuredGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkStructuredPoints.h>
#include <vtkImageData.h>
#include <vtkUniformGrid.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredPoints.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>

#include <vtkCompositeDataSet.h>
#if VTK_MAJOR_VERSION < 6
#include <vtkTemporalDataSet.h>
#define HAVE_VTK_TEMP
#endif
#include <vtkMultiPieceDataSet.h>

#include "vtkToVistle.h"
#include <vistle/core/coords.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/uniformgrid.h>



#ifdef SENSEI
#include <vistle/insitu/sensei/sensei.h>
#define CREATE_VISTLE_OBJECT(type, ...) senseiAdapter.createVistleObject<type>(__VA_ARGS__)
#else
#define CREATE_VISTLE_OBJECT(type, ...) std::make_shared<type>(__VA_ARGS__)
#endif // SENSEI
#define COMMA ,

namespace vistle {
namespace vtk {

namespace {


Object::ptr vtkUGrid2Vistle(SENSEI_ARGUMENT vtkUnstructuredGrid* vugrid, bool checkConvex) {
    Index ncoord = vugrid->GetNumberOfPoints();
    Index nelem = vugrid->GetNumberOfCells();
    Index nconn = 0;
    vtkCellArray* vcellarray = vugrid->GetCells();
    if (vcellarray) {
        nconn = vcellarray->GetNumberOfConnectivityEntries() - nelem;
    }

    UnstructuredGrid::ptr cugrid = CREATE_VISTLE_OBJECT(UnstructuredGrid, nelem, nconn, ncoord);

    float* xc = cugrid->x().data();
    float* yc = cugrid->y().data();
    float* zc = cugrid->z().data();
    Index* elems = cugrid->el().data();
    Index* connlist = cugrid->cl().data();
    Byte* typelist = cugrid->tl().data();

    for (Index i = 0; i < ncoord; ++i) {
        xc[i] = vugrid->GetPoint(i)[0];
        yc[i] = vugrid->GetPoint(i)[1];
        zc[i] = vugrid->GetPoint(i)[2];
    }

#if VTK_MAJOR_VERSION >= 7
    const auto* ghostArray = vugrid->GetCellGhostArray();
#endif
    vtkUnsignedCharArray* vtypearray = vugrid->GetCellTypesArray();
    for (Index i = 0; i < nelem; ++i) {
        switch (vtypearray->GetValue(i))
        {
        case VTK_VERTEX:
        case VTK_POLY_VERTEX:
            typelist[i] = UnstructuredGrid::POINT;
            break;
        case VTK_LINE:
        case VTK_POLY_LINE:
            typelist[i] = UnstructuredGrid::BAR;
            break;
        case VTK_TRIANGLE:
            typelist[i] = UnstructuredGrid::TRIANGLE;
            break;
        case VTK_QUAD:
            typelist[i] = UnstructuredGrid::QUAD;
            break;
        case VTK_TETRA:
            typelist[i] = UnstructuredGrid::TETRAHEDRON;
            break;
        case VTK_HEXAHEDRON:
            typelist[i] = UnstructuredGrid::HEXAHEDRON;
            break;
        case VTK_WEDGE:
            typelist[i] = UnstructuredGrid::PRISM;
            break;
        case VTK_PYRAMID:
            typelist[i] = UnstructuredGrid::PYRAMID;
            break;
        case VTK_POLYHEDRON:
            typelist[i] = UnstructuredGrid::POLYHEDRON;
            break;
        default:
            std::cerr << "VTK cell type " << vtypearray->GetValue(i) << " not handled" << std::endl;
            typelist[i] = 0;
            break;
        }
#if VTK_MAJOR_VERSION >= 7
        if (ghostArray && const_cast<vtkUnsignedCharArray*>(ghostArray)->GetValue(i) & vtkDataSetAttributes::DUPLICATECELL) {
            typelist[i] |= UnstructuredGrid::GHOST_BIT;
        }
#endif
    }

    if (vcellarray) {
        vcellarray->InitTraversal();
        Index k = 0;
        for (Index i = 0; i < nelem; ++i) {
            elems[i] = k;

            vtkIdType npts = 0;
            vtkIdType* pts = nullptr;
            vcellarray->GetNextCell(npts, pts);
            if (typelist[i] == UnstructuredGrid::VPOLYHEDRON) {
                Index j = 0;
                Index nface = pts[j];
                ++j;
                for (Index f = 0; f < nface; ++f) {
                    Index nvert = pts[j];
                    ++j;
                    connlist[k] = nvert;
                    ++k;
                    for (Index v = 0; v < nvert; ++v) {
                        connlist[k] = pts[j];
                        ++k;
                        ++j;
                    }
                }
            }
            else if (typelist[i] == UnstructuredGrid::CPOLYHEDRON) {
                Index j = 0;
                Index nface = pts[j];
                ++j;
                for (Index f = 0; f < nface; ++f) {
                    Index nvert = pts[j];
                    ++j;
                    Index first = pts[j];
                    for (Index v = 0; v < nvert; ++v) {
                        connlist[k] = pts[j];
                        ++k;
                        ++j;
                    }
                    connlist[k] = first;
                    ++k;
                }
            }
            else {
                for (int j = 0; j < npts; ++j) {
                    connlist[k] = pts[j];
                    ++k;
                }
            }
        }
        elems[nelem] = k;
    }

    if (checkConvex) {
        auto nonConvex = cugrid->checkConvexity();
        if (nonConvex > 0) {
            std::cerr << "coVtk::vtkUGrid2Vistle: " << nonConvex << " of " << cugrid->getNumElements() << " cells are non-convex" << std::endl;
        }
    }

    return cugrid;
}

Object::ptr vtkPoly2Vistle(SENSEI_ARGUMENT vtkPolyData* vpolydata) {

    Index ncoord = vpolydata->GetNumberOfPoints();
    Index npolys = vpolydata->GetNumberOfPolys();
    Index nstrips = vpolydata->GetNumberOfStrips();
    Index nlines = vpolydata->GetNumberOfLines();
    Index nverts = vpolydata->GetNumberOfVerts();

    Coords::ptr coords;
        if (npolys > 0)
        {
            Index nstriptris = 0;
            vtkCellArray* strips = vpolydata->GetStrips();
            strips->InitTraversal();
            for (Index i = 0; i < nstrips; ++i)
            {
                vtkIdType npts = 0, * pts = NULL;
                strips->GetNextCell(npts, pts);
                nstriptris += npts - 2;
            }

            vtkCellArray* polys = vpolydata->GetPolys();
            Index ncorner = polys->GetNumberOfConnectivityEntries() - npolys + 3 * nstriptris;
            Polygons::ptr cpoly = CREATE_VISTLE_OBJECT(Polygons, nstriptris + npolys, ncorner, ncoord);
            coords = cpoly;

            Index* cornerlist = cpoly->cl().data();
            Index* polylist = cpoly->el().data();

            Index k = 0;
            polys->InitTraversal();
            for (Index i = 0; i < npolys; ++i)
            {
                polylist[i] = k;

                vtkIdType npts = 0, * pts = NULL;
                polys->GetNextCell(npts, pts);
                for (int j = 0; j < npts; ++j)
                {
                    cornerlist[k] = pts[j];
                    ++k;
                }
            }

            strips->InitTraversal();
            for (Index i = 0; i < nstrips; ++i)
            {
                polylist[npolys + i] = k;
                vtkIdType npts = 0, * pts = NULL;
                strips->GetNextCell(npts, pts);
                for (Index j = 0; j < npts - 2; ++j)
                {
                    if (j % 2)
                    {
                        cornerlist[k++] = pts[j];
                        cornerlist[k++] = pts[j + 1];
                    }
                    else
                    {
                        cornerlist[k++] = pts[j + 1];
                        cornerlist[k++] = pts[j];
                    }
                    cornerlist[k++] = pts[j + 2];
                }
            }
            polylist[nstriptris + npolys] = k;
            assert(k == ncorner);
        }
        else if (nlines > 0)
        {
            vtkCellArray* lines = vpolydata->GetLines();
            Index ncorner = lines->GetNumberOfConnectivityEntries() - nlines;
            Lines::ptr clines = CREATE_VISTLE_OBJECT(Lines, ncoord, ncorner, nlines);
            coords = clines;

            Index* cornerlist = clines->cl().data();
            Index* linelist = clines->el().data();

            Index k = 0;
            lines->InitTraversal();
            for (Index i = 0; i < nlines; ++i)
            {
                linelist[i] = k;

                vtkIdType npts = 0, * pts = NULL;
                lines->GetNextCell(npts, pts);
                for (Index j = 0; j < npts; ++j)
                {
                    cornerlist[k] = pts[j];
                    ++k;
                }
            }
            linelist[nlines] = k;
        }
        else if (nverts > 0)
        {
            if (nverts != ncoord)
                return coords;
            Points::ptr cpoints = CREATE_VISTLE_OBJECT(Points, ncoord);
            coords = cpoints;
        }

    if (coords) {
        float* xc = coords->x().data();
        float* yc = coords->y().data();
        float* zc = coords->z().data();

        for (Index i = 0; i < ncoord; ++i)
        {
            xc[i] = vpolydata->GetPoint(i)[0];
            yc[i] = vpolydata->GetPoint(i)[1];
            zc[i] = vpolydata->GetPoint(i)[2];
        }
    }

    return coords;
}

Object::ptr vtkSGrid2Vistle(SENSEI_ARGUMENT vtkStructuredGrid* vsgrid)
{
    int dim[3];
    vsgrid->GetDimensions(dim);

    StructuredGrid::ptr csgrid = CREATE_VISTLE_OBJECT(StructuredGrid, dim[0], dim[1], dim[2]);
    float* xc = csgrid->x().data();
    float* yc = csgrid->y().data();
    float* zc = csgrid->z().data();

    Index l = 0;
    for (Index i = 0; i < Index(dim[0]); ++i)
    {
        for (Index j = 0; j < Index(dim[1]); ++j)
        {
            for (Index k = 0; k < Index(dim[2]); ++k)
            {
                Index idx = k * (dim[0] * dim[1]) + j * dim[0] + i;
                xc[l] = vsgrid->GetPoint(idx)[0];
                yc[l] = vsgrid->GetPoint(idx)[1];
                zc[l] = vsgrid->GetPoint(idx)[2];
                ++l;
            }
        }
    }

    return csgrid;
}

Object::ptr vtkImage2Vistle(SENSEI_ARGUMENT vtkImageData* vimage)
{
    int n[3];
    vimage->GetDimensions(n);
    double origin[3] = { 0., 0., 0. };
    vimage->GetOrigin(origin);
    double spacing[3] = { 1., 1., 1. };
    vimage->GetSpacing(spacing);
    UniformGrid::ptr ug = CREATE_VISTLE_OBJECT(UniformGrid, n[0], n[1], n[2]);
    for (int c = 0; c < 3; ++c) {
        ug->min()[c] = origin[c];
        ug->max()[c] = origin[c] + (n[c] - 1) * spacing[c];
    }
    return ug;
}

Object::ptr vtkUniGrid2Vistle(SENSEI_ARGUMENT vtkUniformGrid* vugrid)
{
    int n[3];
    vugrid->GetDimensions(n);
    double origin[3] = { 0., 0., 0. };
    vugrid->GetOrigin(origin);
    double spacing[3] = { 1., 1., 1. };
    vugrid->GetSpacing(spacing);
    UniformGrid::ptr ug = CREATE_VISTLE_OBJECT(UniformGrid, n[0], n[1], n[2]);
    for (int c = 0; c < 3; ++c) {
        ug->min()[c] = origin[c];
        ug->max()[c] = origin[c] + (n[c] - 1) * spacing[c];
    }
    return ug;
}

Object::ptr vtkRGrid2Vistle(SENSEI_ARGUMENT vtkRectilinearGrid* vrgrid)
{
    int n[3];
    vrgrid->GetDimensions(n);

    RectilinearGrid::ptr rgrid = CREATE_VISTLE_OBJECT(RectilinearGrid, n[0], n[1], n[2]);
    Scalar* c[3];
    for (int i = 0; i < 3; ++i) {
        c[i] = rgrid->coords(i).data();
    }

    vtkDataArray* vx = vrgrid->GetXCoordinates();
    for (Index i = 0; i < Index(n[0]); ++i)
        c[0][i] = vx->GetTuple1(i);

    vtkDataArray* vy = vrgrid->GetYCoordinates();
    for (Index i = 0; i < Index(n[1]); ++i)
        c[1][i] = vy->GetTuple1(i);

    vtkDataArray* vz = vrgrid->GetZCoordinates();
    for (Index i = 0; i < Index(n[2]); ++i)
        c[2][i] = vz->GetTuple1(i);

    return rgrid;
}

template <typename ValueType, class vtkType>
DataBase::ptr vtkArray2Vistle(SENSEI_ARGUMENT vtkType* vd, Object::const_ptr grid)
{
    bool perCell = false;
    const Index n = vd->GetNumberOfTuples();
    std::array<Index, 3> dim = { n, 1, 1 };
    if (auto sgrid = StructuredGridBase::as(grid)) {
        for (int c = 0; c < 3; ++c)
            dim[c] = sgrid->getNumDivisions(c);
    }
    if (n > 0 && n == (dim[0] - 1) * (dim[1] - 1) * (dim[2] - 1)) {
        perCell = true;
        // cell-mapped data
    }
    auto numGridDivisions = dim;
    if (perCell)
    {
        for (int c = 0; c < 3; ++c)
            dim[c] > 1 ? --dim[c] : dim[c];
    }
    switch (vd->GetNumberOfComponents())
    {
    case 1:
    {
        Vec<Scalar, 1>::ptr cf = CREATE_VISTLE_OBJECT(Vec<Scalar COMMA 1>, n);
        float* x = cf->x().data();
        Index l = 0;
        for (Index k = 0; k < dim[2]; ++k) {
            for (Index j = 0; j < dim[1]; ++j) {
                for (Index i = 0; i < dim[0]; ++i) {
                    const Index idx = perCell ? StructuredGridBase::cellIndex(i, j, k, numGridDivisions.data()) : StructuredGridBase::vertexIndex(i, j, k, numGridDivisions.data());
                    x[idx] = vd->GetValue(l);
                    ++l;
                }
            }
        }
        cf->setGrid(grid);
        return cf;
    }
    break;
    case 3:
    {
        Vec<Scalar, 3>::ptr cv = CREATE_VISTLE_OBJECT(Vec<Scalar COMMA 3>, n);
        float* x = cv->x().data();
        float* y = cv->y().data();
        float* z = cv->z().data();
        Index l = 0;
        for (Index k = 0; k < dim[2]; ++k) {
            for (Index j = 0; j < dim[1]; ++j) {
                for (Index i = 0; i < dim[0]; ++i) {
                    const Index idx = perCell ? StructuredGridBase::cellIndex(i, j, k, numGridDivisions.data()) : StructuredGridBase::vertexIndex(i, j, k, numGridDivisions.data());
#if VTK_MAJOR_VERSION < 7 || (VTK_MAJOR_VERSION==7 && VTK_MINOR_VERSION<1)
                    ValueType v[3];
                    vd->GetTupleValue(l, v);
                    x[idx] = v[0];
                    y[idx] = v[1];
                    z[idx] = v[2];
#else
                    x[idx] = vd->GetTypedComponent(l, 0);
                    y[idx] = vd->GetTypedComponent(l, 1);
                    z[idx] = vd->GetTypedComponent(l, 2);
#endif
                    ++l;
                }
            }
        }
        cv->setGrid(grid);
        return cv;
    }
    break;
    }

    return NULL;
}
#ifdef SENSEI
} // anonymous namespace
#endif
DataBase::ptr vtkData2Vistle(SENSEI_ARGUMENT vtkDataArray* varr, Object::const_ptr grid) {

    DataBase::ptr data;

    if (!varr)
        return data;

    const Index n = varr->GetNumberOfTuples();
    Index dim[3] = { n, 1, 1 };
    if (auto sgrid = StructuredGridBase::as(grid)) {
        for (int c = 0; c < 3; ++c) {
            dim[c] = sgrid->getNumDivisions(c);
        }
    }
    if (n > 0 && n == (dim[0] - 1) * (dim[1] - 1) * (dim[2] - 1))
    {
        // cell-mapped data
        for (int c = 0; c < 3; ++c)
            --dim[c];
    }
    if (dim[0] * dim[1] * dim[2] != n)
    {
        std::cerr << "coVtk::vtkData2Vistle: non-matching grid size: [" << dim[0] << "*" << dim[1] << "*" << dim[2] << "] != " << n << std::endl;
        return NULL;
    }

    if (vtkFloatArray* vd = dynamic_cast<vtkFloatArray*>(varr))
    {
        return vtkArray2Vistle<float, vtkFloatArray>(SENSEI_PARAMETER vd, grid);
    }
    else if (vtkDoubleArray* vd = dynamic_cast<vtkDoubleArray*>(varr))
    {
        return vtkArray2Vistle<double, vtkDoubleArray>(SENSEI_PARAMETER vd, grid);
    }
    else if (vtkShortArray* vd = dynamic_cast<vtkShortArray*>(varr))
    {
        return vtkArray2Vistle<short, vtkShortArray>(SENSEI_PARAMETER vd, grid);
    }
    else if (vtkUnsignedShortArray* vd = dynamic_cast<vtkUnsignedShortArray*>(varr))
    {
        return vtkArray2Vistle<unsigned short, vtkUnsignedShortArray>(SENSEI_PARAMETER vd, grid);
    }
    else if (vtkUnsignedIntArray* vd = dynamic_cast<vtkUnsignedIntArray*>(varr))
    {
        return vtkArray2Vistle<unsigned int, vtkUnsignedIntArray>(SENSEI_PARAMETER vd, grid);
    }
    return NULL;
}


#ifndef SENSEI
} // anonymous namespace
#endif

vistle::Object::ptr toGrid(SENSEI_ARGUMENT vtkDataObject* vtk, bool checkConvex) {

    if (vtkUnstructuredGrid* vugrid = dynamic_cast<vtkUnstructuredGrid*>(vtk))
        return vtkUGrid2Vistle(SENSEI_PARAMETER vugrid, checkConvex);

    if (vtkPolyData* vpolydata = dynamic_cast<vtkPolyData*>(vtk))
        return vtkPoly2Vistle(SENSEI_PARAMETER vpolydata);

    if (vtkStructuredGrid* vsgrid = dynamic_cast<vtkStructuredGrid*>(vtk))
        return vtkSGrid2Vistle(SENSEI_PARAMETER vsgrid);

    if (vtkRectilinearGrid* vrgrid = dynamic_cast<vtkRectilinearGrid*>(vtk))
        return vtkRGrid2Vistle(SENSEI_PARAMETER vrgrid);

    if (vtkUniformGrid* vugrid = dynamic_cast<vtkUniformGrid*>(vtk))
        return vtkUniGrid2Vistle(SENSEI_PARAMETER vugrid);

    if (vtkImageData* vimage = dynamic_cast<vtkImageData*>(vtk))
        return vtkImage2Vistle(SENSEI_PARAMETER vimage);

    return nullptr;
}

vistle::DataBase::ptr getField(SENSEI_ARGUMENT vtkDataSetAttributes* vtk, const std::string& name, Object::const_ptr grid) {

    DataBase::ptr result;

    auto varr = vtk->GetScalars(name.c_str());
    if (!varr)
        varr = vtk->GetVectors(name.c_str());
    if (!varr)
        varr = vtk->GetNormals(name.c_str());
    if (!varr)
        varr = vtk->GetTCoords(name.c_str());
    if (!varr)
        varr = vtk->GetTensors(name.c_str());

    return vtkData2Vistle(SENSEI_PARAMETER varr, grid);
}

vistle::DataBase::ptr getField(SENSEI_ARGUMENT vtkFieldData* vtk, const std::string& name, Object::const_ptr grid) {

    DataBase::ptr result;
    auto varr = vtk->GetArray(name.c_str());
    return vtkData2Vistle(SENSEI_PARAMETER varr, grid);
}

} // namespace vtk

} // namespace vistle
