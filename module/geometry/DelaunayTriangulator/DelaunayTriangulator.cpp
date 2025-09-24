#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/triangles.h>
#include <vistle/alg/objalg.h>
#include <vistle/util/enum.h>


#include "DelaunayTriangulator.h"
#include <tetgen.h>

#ifdef HAVE_CGAL
#include <boost/next_prior.hpp>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Triangulation_3.h>
#include <CGAL/Delaunay_triangulation_cell_base_3.h>
#include <CGAL/Triangulation_vertex_base_with_info_3.h>

#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_2.h>
#include <CGAL/Triangulation_face_base_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>

namespace cgal = CGAL;
#endif


DEFINE_ENUM_WITH_STRING_CONVERSIONS(Method, (Tetrahedra)(ConvexHull)(Surface_YZ)(Surface_XZ)(Surface_XY))


MODULE_MAIN(DelaunayTriangulator)

using namespace vistle;

template<unsigned Dim>
auto convertToTetGen(Points::const_ptr points, typename Vec<vistle::Scalar, Dim>::const_ptr data)
{
    auto in = std::make_unique<tetgenio>();
    in->numberofpoints = points->getSize();
    in->pointlist = new REAL[in->numberofpoints * 3];
    for (size_t i = 0; i < (size_t)in->numberofpoints; ++i) {
        in->pointlist[3 * i] = points->x()[i];
        in->pointlist[3 * i + 1] = points->y()[i];
        in->pointlist[3 * i + 2] = points->z()[i];
    }
    if (data) {
        in->numberofpointattributes = Dim;
        in->pointattributelist = new REAL[in->numberofpoints * Dim];
        for (size_t i = 0; i < (size_t)in->numberofpoints; ++i) {
            in->pointattributelist[i * Dim] = data->x()[i];
            if (Dim > 1)
                in->pointattributelist[i * Dim + 1] = data->y()[i];
            if (Dim > 2)
                in->pointattributelist[i * Dim + 2] = data->z()[i];
            if (Dim > 3)
                in->pointattributelist[i * Dim + 3] = data->w()[i];
        }
    }
    return in;
}

template<unsigned Dim>
auto convertFromTetGen(const tetgenio &out)
{
    std::pair<UnstructuredGrid::ptr, typename Vec<Scalar, Dim>::ptr> retval;

    size_t numElements = out.numberoftetrahedra;
    size_t numCorners = numElements * out.numberofcorners;
    size_t numVertices = out.numberofpoints;
    auto &grid = retval.first = std::make_shared<UnstructuredGrid>(numElements, numCorners, numVertices);
    for (size_t i = 0; i < numVertices; ++i) {
        grid->x().data()[i] = out.pointlist[i * 3];
        grid->y().data()[i] = out.pointlist[i * 3 + 1];
        grid->z().data()[i] = out.pointlist[i * 3 + 2];
    }

    for (size_t i = 0; i < numElements; ++i) {
        for (int j = 0; j < out.numberofcorners; ++j) {
            grid->cl().data()[i * out.numberofcorners + j] = out.tetrahedronlist[i * out.numberofcorners + j];
        }
        grid->tl().data()[i] = UnstructuredGrid::TETRAHEDRON;
        grid->el().data()[i] = i * out.numberofcorners;
    }
    grid->el().data()[numElements] = numElements * out.numberofcorners;
    if (out.numberofpointattributes == Dim) {
        auto &retScalar = retval.second = std::make_shared<Vec<Scalar, Dim>>(numVertices);
        for (size_t i = 0; i < numVertices; ++i) {
            retScalar->x()[i * Dim] = out.pointattributelist[i];
            if (Dim > 1)
                retScalar->y()[i] = out.pointattributelist[i * Dim + 1];
            if (Dim > 2)
                retScalar->z()[i] = out.pointattributelist[i * Dim + 2];
            if (Dim > 3)
                retScalar->w()[i] = out.pointattributelist[i * Dim + 3];
        }
    }
    return retval;
}


#ifdef HAVE_CGAL
// 3D Delaunay triangulation
typedef cgal::Filtered_kernel<cgal::Simple_cartesian<vistle::Scalar>> K;
//typedef cgal::Exact_predicates_inexact_constructions_kernel K;

typedef cgal::Triangulation_vertex_base_with_info_3<vistle::Index, K> Vb3;
typedef cgal::Delaunay_triangulation_cell_base_3<K> Cb3;
typedef cgal::Triangulation_data_structure_3<Vb3, Cb3> Tds3;
typedef cgal::Delaunay_triangulation_3<K, Tds3> Delaunay3;
typedef Delaunay3::Point Point3;
typedef Tds3::Cell_handle Cell_handle3;
typedef Tds3::Vertex_handle Vertex_handle3;

// a functor that returns a std::pair<Point,vistle::Index>,
//the Index is used to look up the coordinates for the Point
struct MakePoint3: public CGAL::cpp98::unary_function<vistle::Index, std::pair<Point3, vistle::Index>> {
    const vistle::Scalar *x, *y, *z;
    MakePoint3(const vistle::Scalar *x, const vistle::Scalar *y, const vistle::Scalar *z): x(x), y(y), z(z) {}
    std::pair<Point3, vistle::Index> operator()(vistle::Index i) const
    {
        return std::make_pair(Point3(x[i], y[i], z[i]), i);
    }
};


// projected 2D Delaunay triangulation
typedef cgal::Triangulation_vertex_base_with_info_2<vistle::Index, K> Vb2;
typedef cgal::Triangulation_face_base_2<K> Fb2;
typedef cgal::Triangulation_data_structure_2<Vb2, Fb2> Tds2;
typedef cgal::Delaunay_triangulation_2<K, Tds2> Delaunay2;
typedef Delaunay2::Point Point2;
typedef Tds2::Face_handle Face_handle2;
typedef Tds2::Vertex_handle Vertex_handle2;

struct MakePoint2: public CGAL::cpp98::unary_function<vistle::Index, std::pair<Point2, vistle::Index>> {
    const vistle::Scalar *x, *y;
    MakePoint2(const vistle::Scalar *x, const vistle::Scalar *y): x(x), y(y) {}
    std::pair<Point2, vistle::Index> operator()(vistle::Index i) const { return std::make_pair(Point2(x[i], y[i]), i); }
};

Triangles::ptr computeConvexHull(Points::const_ptr &points)
{
    const auto *x = points->x().data();
    const auto *y = points->y().data();
    const auto *z = points->z().data();

    Delaunay3 T(
        boost::make_transform_iterator(boost::counting_iterator<Index>(0), MakePoint3(x, y, z)),
        boost::make_transform_iterator(boost::counting_iterator<Index>(points->getSize()), MakePoint3(x, y, z)));

    auto tds = T.tds();
    auto inf = T.infinite_vertex();
    std::vector<Cell_handle3> cells;
    cells.reserve(64);
    T.incident_cells(inf, std::back_inserter(cells));

    auto tri = std::make_shared<Triangles>(cells.size() * 3, 0);
    tri->resetCoords();
    tri->d()->x[0] = points->d()->x[0];
    tri->d()->x[1] = points->d()->x[1];
    tri->d()->x[2] = points->d()->x[2];
    auto *cl = tri->cl().data();

    Index nelem = 0;
    for (typename std::vector<Cell_handle3>::iterator cit = cells.begin(); cit != cells.end(); ++cit) {
        const auto &ch = *cit;
        const auto idx = ch->index(inf);
        int nvert = 0;
        Index verts[3];
        if (idx == 0 || idx == 2) {
            for (int i = 3; i >= 0; --i) {
                if (i == idx)
                    continue;
                auto v = ch->vertex(i);
                if (v == Vertex_handle3())
                    break;
                verts[nvert++] = v->info();
            }
        } else {
            for (int i = 0; i < 4; ++i) {
                if (i == idx)
                    continue;
                auto v = ch->vertex(i);
                if (v == Vertex_handle3())
                    break;
                verts[nvert++] = v->info();
            }
        }
        if (nvert != 3)
            continue;
        ++nelem;
        for (int i = 0; i < 3; ++i) {
            *cl = verts[i];
            ++cl;
        }
    }
    tri->cl().resize(nelem * 3);
    if (nelem == 0)
        tri->resetCoords();
    return tri;
}

Triangles::ptr computeTri(Points::const_ptr &points, Method projAxis)
{
    const auto *x = points->x().data();
    const auto *y = points->y().data();
    const auto *z = points->z().data();

    const vistle::Scalar *X = x, *Y = y;
    switch (projAxis) {
    case Surface_YZ:
        X = y;
        Y = z;
        break;
    case Surface_XZ:
        X = x;
        Y = z;
        break;
    case Surface_XY:
        X = x;
        Y = y;
        break;
    default:
        assert("you should never get here" == 0);
        break;
    }

    Delaunay2 T(boost::make_transform_iterator(boost::counting_iterator<Index>(0), MakePoint2(X, Y)),
                boost::make_transform_iterator(boost::counting_iterator<Index>(points->getSize()), MakePoint2(X, Y)));

    auto tds = T.tds();
    auto inf = T.infinite_vertex();
    auto begin = T.faces_begin();
    auto end = T.faces_end();
    vistle::Index nfaces = 0;
    for (auto fit = begin; fit != end; ++fit) {
        const auto &fh = *fit;
        // skip outer faces
        if (fh.has_vertex(inf))
            continue;
        ++nfaces;
    }

    auto tri = std::make_shared<Triangles>(nfaces * 3, 0);
    if (nfaces > 0) {
        tri->resetCoords();
        tri->d()->x[0] = points->d()->x[0];
        tri->d()->x[1] = points->d()->x[1];
        tri->d()->x[2] = points->d()->x[2];
        auto *cl = tri->cl().data();

        for (auto fit = begin; fit != end; ++fit) {
            const auto &fh = *fit;
            if (fh.has_vertex(inf))
                continue;
            for (unsigned i = 0; i < 3; ++i) {
                *cl = fh.vertex(i)->info();
                ++cl;
            }
        }
    }

    return tri;
}
#else
Triangles::ptr computeConvexHull(Points::const_ptr &points)
{
    return Triangles::ptr();
}

Triangles::ptr computeTri(Points::const_ptr &points, Method projAxis)
{
    return Triangles::ptr();
}
#endif

template<unsigned Dim>
Object::ptr DelaunayTriangulator::calculateGrid(Points::const_ptr points,
                                                typename Vec<vistle::Scalar, Dim>::const_ptr data) const
{
    const auto method = static_cast<Method>(m_methodChoice->getValue());
    if (method == Tetrahedra) {
        auto in = convertToTetGen<Dim>(points, data);
        tetgenio out;
        tetgenbehavior behaviour;
        try {
            tetrahedralize(&behaviour, in.get(), &out);
        } catch (int &e) {
            const char *err = "unknown error";
            switch (e) {
                // copied from tetgen.h: terminatetetgen(...) and adapted
            case 1: // Out of memory.
                err = "Error:  Out of memory.";
                break;
            case 2: // Encounter an internal error.
                err = "Please report this bug to Hang.Si@wias-berlin.de. Include\n"
                      "  the message above, your input data set, and the exact\n"
                      "  command line you used to run this program, thank you.";
                break;
            case 3:
                err = "The input surface mesh contain self-intersections. Program stopped.";
                break;
            case 4:
                err = "A very small input feature size was detected. Program stopped.";
                break;
            case 5:
                err = "Two very close input facets were detected. Program stopped.\n"
                      "Hint: use -Y option to avoid adding Steiner points in boundary.";
                break;
            case 10:
                err = "An input error was detected. Program stopped.";
                break;
            case 200:
                err = "Boundary contains Steiner points (-YY option). Program stopped.";
                break;
            }

            std::stringstream ss;
            ss << "tetrahedralize failed: error code " << e << ": " << err;
            sendError(ss.str());
            return Object::ptr();
        }
        if (out.numberofpoints == in->numberofpoints) //we can reuse the data object
        {
            auto grid = convertFromTetGen<Dim>(out).first;
            updateMeta(grid);
            if (data) {
                auto clone = data->clone();
                updateMeta(clone);
                clone->setGrid(grid);
                return clone;
            }
            return grid;
        } else {
            auto gridAndData = convertFromTetGen<Dim>(out);
            updateMeta(gridAndData.first);
            if (auto &remappedData = gridAndData.second) //data is remapped
            {
                remappedData->copyAttributes(data);
                remappedData->setGrid(gridAndData.first);
                updateMeta(remappedData);
                return remappedData;
            }
            return gridAndData.first;
        }
    } else {
        Triangles::ptr tri;
        if (method == ConvexHull) {
            tri = computeConvexHull(points);
        } else if (method == Surface_YZ || method == Surface_XZ || method == Surface_XY) {
            tri = computeTri(points, method);
        } else {
            return Object::ptr();
        }
        updateMeta(tri);
        if (data) {
            auto clone = data->clone();
            updateMeta(clone);
            clone->setGrid(tri);
            return clone;
        }
        return tri;
    }
    return Object::ptr();
}


DelaunayTriangulator::DelaunayTriangulator(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    auto pin = createInputPort("points_in", "unconnected points");
    auto pout = createOutputPort("grid_out", "grid or data with grid");
    linkPorts(pin, pout);
    m_methodChoice = addIntParameter("method", "algorithm to perform on input vertices", ConvexHull, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_methodChoice, Method);
}

DelaunayTriangulator::~DelaunayTriangulator()
{}

bool DelaunayTriangulator::changeParameter(const vistle::Parameter *param)
{
#ifndef HAVE_CGAL
    if (param == m_methodChoice) {
        if (m_methodChoice->getValue() != Tetrahedra) {
            sendError("CGAL not available, only Tetrahedra method is supported");
        }
    }
#endif
    return Module::changeParameter(param);
}

bool DelaunayTriangulator::compute()
{
    auto obj = expect<Object>("points_in");
    auto split = splitContainerObject(obj);
    if (!split.geometry) {
        sendError("no input geometry");
        return true;
    }

    if (auto data = split.mapped) {
        if (data->guessMapping() != DataBase::Vertex) {
            sendError("data has to be mapped per vertex");
            return true;
        }
    }

    Points::const_ptr points = Points::as(split.geometry);
    if (!points) {
        sendError("cannot get underlying points");
        return true;
    }

    vistle::Object::ptr grid, out;
    if (auto scalar = Vec<vistle::Scalar>::as(split.mapped)) {
        out = calculateGrid<1>(points, scalar);
        updateMeta(out);
        if (auto d = DataBase::as(out)) {
            grid = std::const_pointer_cast<Object>(d->grid());
        }
    } else if (auto vector = Vec<vistle::Scalar, 3>::as(split.mapped)) {
        out = calculateGrid<3>(points, vector);
        if (auto d = DataBase::as(out)) {
            grid = std::const_pointer_cast<Object>(d->grid());
        }
    } else if (split.mapped) {
        sendError("unsupported mapped data type");
        return true;
    }

    auto d = accept<DataBase>("points_in");
    if (auto entry = m_results.getOrLock(split.geometry->getName(), grid)) {
        if (!split.mapped) {
            grid = calculateGrid<1>(points, nullptr);
            out = grid;
        }
        m_results.storeAndUnlock(entry, grid);
    } else {
        if (auto d = DataBase::as(out)) {
            // reuse grid from previous triangulation of identical input points
            d->setGrid(grid);
        } else {
            out = grid;
        }
    }

    addObject("grid_out", out);

    return true;
}
