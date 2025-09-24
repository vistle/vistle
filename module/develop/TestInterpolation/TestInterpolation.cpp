#include <vistle/core/object.h>
#include <vistle/core/grid.h>
#include <vistle/core/message.h>
#include <vistle/core/celltree.h>
#include <vistle/core/vec.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include "TestInterpolation.h"
#include <vistle/util/enum.h>
#include <random>

MODULE_MAIN(TestInterpolation)

using namespace vistle;

TestInterpolation::TestInterpolation(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("data_in", "grid");

    m_count = addIntParameter("count", "number of random points to generate per block", 100);
    m_createCelltree = addIntParameter("create_celltree", "create celltree", 0, Parameter::Boolean);
    m_mode = addIntParameter("mode", "interpolation mode", GridInterface::Linear, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_mode, InterpolationMode, GridInterface);
}

TestInterpolation::~TestInterpolation()
{}

namespace {
Vector3 randpoint(const Vector3 &min, const Vector3 &max)
{
    auto rand = std::minstd_rand0();
    auto dist = std::uniform_real_distribution<Scalar>();

    Vector3 point;
    for (int c = 0; c < 3; ++c) {
        point[c] = dist(rand) * (max[c] - min[c]) + min[c];
    }
    return point;
}
} // namespace

bool TestInterpolation::compute()
{
    const Index count = getIntParameter("count");
    const GridInterface::InterpolationMode mode = (GridInterface::InterpolationMode)m_mode->getValue();

    auto obj = expect<Object>("data_in");
    if (!obj) {
        sendError("no valid object");
        return true;
    }

    const GridInterface *grid = obj->getInterface<GridInterface>();
    if (!grid) {
        sendError("structured or unstructured grid required");
        return true;
    }

    if (m_createCelltree->getValue()) {
        if (auto celltree = grid->object()->getInterface<CelltreeInterface<3>>()) {
            if (!celltree->hasCelltree()) {
                celltree->getCelltree();
                if (!celltree->validateCelltree()) {
                    sendInfo("celltree validation failed for block %d", (int)grid->object()->getBlock());
                }
            }
        }
    }

    auto bounds = grid->getBounds();
    Vector3 min = bounds.first, max = bounds.second;
    std::vector<Scalar> xx, yy, zz;
    const Scalar *x = nullptr, *y = nullptr, *z = nullptr;
    if (auto v3 = Vec<Scalar, 3>::as(grid->object())) {
        x = v3->x().data();
        y = v3->y().data();
        z = v3->z().data();
    } else if (auto uni = UniformGrid::as(grid->object())) {
        const Index nvert = uni->getNumDivisions(0) * uni->getNumDivisions(1) * uni->getNumDivisions(2);
        xx.resize(nvert);
        yy.resize(nvert);
        zz.resize(nvert);
        x = xx.data();
        y = yy.data();
        z = zz.data();
        Scalar *x0 = xx.data();
        Scalar *y0 = yy.data();
        Scalar *z0 = zz.data();

        Vector3 dist = max - min;
        for (int c = 0; c < 3; ++c) {
            dist[c] /= uni->getNumDivisions(c) - 1;
        }
        const Index dim[3] = {uni->getNumDivisions(0), uni->getNumDivisions(1), uni->getNumDivisions(2)};
        for (Index i = 0; i < dim[0]; i++) {
            for (Index j = 0; j < dim[1]; j++) {
                for (Index k = 0; k < dim[2]; k++) {
                    const Index idx = UniformGrid::vertexIndex(i, j, k, dim);
                    x0[idx] = min.x() + i * dist.x();
                    y0[idx] = min.y() + j * dist.y();
                    z0[idx] = min.z() + k * dist.z();
                }
            }
        }
    } else if (auto rect = RectilinearGrid::as(grid->object())) {
        const Index nvert = rect->getNumDivisions(0) * rect->getNumDivisions(1) * rect->getNumDivisions(2);
        xx.resize(nvert);
        yy.resize(nvert);
        zz.resize(nvert);
        x = xx.data();
        y = yy.data();
        z = zz.data();
        Scalar *x0 = xx.data();
        Scalar *y0 = yy.data();
        Scalar *z0 = zz.data();
        const Index dim[3] = {rect->getNumDivisions(0), rect->getNumDivisions(1), rect->getNumDivisions(2)};
        for (Index i = 0; i < dim[0]; i++) {
            for (Index j = 0; j < dim[1]; j++) {
                for (Index k = 0; k < dim[2]; k++) {
                    const Index idx = RectilinearGrid::vertexIndex(i, j, k, dim);
                    x0[idx] = rect->coords(0)[i];
                    y0[idx] = rect->coords(1)[j];
                    z0[idx] = rect->coords(2)[k];
                }
            }
        }
    }

    Index numChecked = 0;
    Scalar squaredError = 0;
    for (Index i = 0; i < count; ++i) {
        Vector3 point(randpoint(min, max));
        Index idx = grid->findCell(point);
        if (idx != InvalidIndex) {
            ++numChecked;
            GridInterface::Interpolator interpol = grid->getInterpolator(idx, point, DataBase::Vertex, mode);
            Vector3 p = interpol(x, y, z);
            Scalar d2 = (point - p).squaredNorm();
            if (d2 > 0.01) {
                std::cerr << "point: " << point.transpose() << ", recons: " << p.transpose() << std::endl;
            }
            squaredError += d2;
        }
    }
    std::cerr << "block " << grid->object()->getBlock() << ", bounds: min " << min.transpose() << ", max "
              << max.transpose() << ", checked: " << numChecked << ", avg error: " << squaredError / numChecked
              << std::endl;

    return true;
}
