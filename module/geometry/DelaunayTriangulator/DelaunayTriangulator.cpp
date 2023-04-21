#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/alg/objalg.h>


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
    std::pair<UnstructuredGrid::ptr, typename Vec<Scalar, Dim>::ptr> retval;

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
        auto grid = convertFromTetGen<Dim>(out).first;
        updateMeta(grid);
        if (data) {
            auto clone = data->clone();
            clone->setGrid(grid);
            updateMeta(clone);
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
    if (auto scalar = Vec<Scalar>::as(split.mapped)) {
        out = calculateGrid<1>(points, scalar);
        updateMeta(out);
        if (auto d = DataBase::as(out)) {
            grid = std::const_pointer_cast<Object>(d->grid());
        }
    } else if (auto vector = Vec<Scalar, 3>::as(split.mapped)) {
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
