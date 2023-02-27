#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>


#include "DelaunayTriangulator.h"
#include <tetgen.h>
MODULE_MAIN(DelaunayTriangulator)

using namespace vistle;

template<unsigned Dim>
auto convertToTetGen(Points::const_ptr points, typename Vec<Scalar, Dim>::const_ptr data)
{
    auto in = std::make_unique<tetgenio>();
    in->numberofpoints = points->getSize();
    in->pointlist = new REAL[in->numberofpoints * 3];
    for (size_t i = 0; i < in->numberofpoints; ++i) {
        in->pointlist[3 * i] = points->x()[i];
        in->pointlist[3 * i + 1] = points->y()[i];
        in->pointlist[3 * i + 2] = points->z()[i];
    }
    if (data) {
        in->numberofpointattributes = Dim;
        in->pointattributelist = new REAL[in->numberofpoints * Dim];
        for (size_t i = 0; i < in->numberofpoints; ++i) {
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
    std::pair<UnstructuredGrid::ptr, Vec<Scalar, Dim>::ptr> retval;

    size_t numElements = out.numberoftetrahedra;
    size_t numCorners = numElements * out.numberofcorners;
    size_t numVertices = out.numberofpoints;
    auto &grid = retval.first = make_ptr<UnstructuredGrid>(numElements, numCorners, numVertices);
    for (size_t i = 0; i < numVertices; ++i) {
        grid->x().data()[i] = out.pointlist[i * 3];
        grid->y().data()[i] = out.pointlist[i * 3 + 1];
        grid->z().data()[i] = out.pointlist[i * 3 + 2];
    }

    for (size_t i = 0; i < numElements; ++i) {
        for (size_t j = 0; j < out.numberofcorners; ++j) {
            grid->cl().data()[i * out.numberofcorners + j] = out.tetrahedronlist[i * out.numberofcorners + j];
        }
        grid->tl().data()[i] = UnstructuredGrid::TETRAHEDRON;
        grid->el().data()[i] = i * out.numberofcorners;
    }
    grid->el().data()[numElements] = numElements * out.numberofcorners;
    if (out.numberofpointattributes == Dim) {
        auto &retScalar = retval.second = make_ptr<Vec<Scalar, Dim>>(numVertices);
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

template<unsigned Dim>
Object::ptr DelaunayTriangulator::calculateGrid(Points::const_ptr points, typename Vec<Scalar, Dim>::const_ptr data)
{
    auto in = convertToTetGen<Dim>(points, data);
    tetgenio out;
    tetgenbehavior behaviour;
    tetrahedralize(&behaviour, in.get(), &out);
    if (out.numberofpoints == in->numberofpoints) //we can reuse the data object
    {
        auto m_grid = convertFromTetGen<Dim>(out).first;
        updateMeta(m_grid);
        m_points = points;
        if (data) {
            auto clone = data->clone();
            clone->setGrid(m_grid);
            updateMeta(clone);
            return clone;
        }
        return m_grid;
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
}

DelaunayTriangulator::DelaunayTriangulator(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("points_in", "unconnected points");
    createOutputPort("grid_out", "grid or data with grid");
}

DelaunayTriangulator::~DelaunayTriangulator()
{}

bool DelaunayTriangulator::compute()
{
    auto d = accept<DataBase>("points_in");
    Points::const_ptr points;
    if (auto points = Points::as(d)) {
        addObject("grid_out", calculateGrid<1>(points, nullptr));
    } else if (auto points = Points::as(d->grid())) {
        if (m_grid && m_points && *m_points == *points) {
            auto clone = d->clone();
            clone->setGrid(m_grid);
            updateMeta(clone);
            addObject("grid_out", clone);
        } else if (auto scalar = Vec<Scalar>::as(d)) {
            addObject("grid_out", calculateGrid<1>(points, scalar));
        } else if (auto vector = Vec<Scalar, 3>::as(d)) {
            addObject("grid_out", calculateGrid<3>(points, vector));
        }
    }
    return true;
}
