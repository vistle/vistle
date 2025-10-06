#include <vistle/core/object.h>
#include <vistle/core/points.h>
#include <vistle/alg/objalg.h>

#include "ApplyTransform.h"

#ifdef APPLYTRANSFORMVTKM
#include <viskores/cont/DataSet.h>
#include <viskores/filter/field_transform/PointTransform.h>

#include <vistle/vtkm/convert.h>
#endif

MODULE_MAIN(ApplyTransform)

using namespace vistle;

ApplyTransform::ApplyTransform(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("grid_in", "grid or geometry");
    createOutputPort("grid_out", "unconnected points");

    addResultCache(m_gridCache);
    addResultCache(m_resultCache);
}

ApplyTransform::~ApplyTransform()
{}

bool ApplyTransform::compute()
{
    auto o = expect<Object>("grid_in");
    auto split = splitContainerObject(o);
    auto coords = Coords::as(split.geometry);
    if (!coords) {
        sendError("no coordinates on input");
        return true;
    }

    Object::ptr out;
    if (auto resultEntry = m_resultCache.getOrLock(o->getName(), out)) {
        Object::ptr outGrid;
        if (auto gridEntry = m_gridCache.getOrLock(coords->getName(), outGrid)) {
            auto T = coords->getTransform();
            if (T.isIdentity()) {
                outGrid = coords->clone();
            } else {
#ifdef APPLYTRANSFORMVTKM
                viskores::cont::DataSet inDs;
                vistle::vtkmSetGrid(inDs, coords);

                // Copy Vistle Matrix4 into Viskores 4x4
                viskores::Matrix<vistle::Scalar, 4, 4> M;
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 4; ++c)
                        M[r][c] = T(r, c);

                viskores::filter::field_transform::PointTransform filter;
                filter.SetTransform(M);
                auto outDs = filter.Execute(inDs);
                outGrid = vistle::vtkmGetGeometry(outDs);
#else
                auto clone = coords->clone();
                clone->resetCoords();
                Index ncoords = coords->getSize();
                clone->setSize(ncoords);
                auto *x = coords->x().data(), *y = coords->y().data(), *z = coords->z().data();
                auto *nx = clone->x().data(), *ny = clone->y().data(), *nz = clone->z().data();
                for (Index i = 0; i < ncoords; ++i) {
                    auto p = T * Vector4(x[i], y[i], z[i], Scalar(1));
                    nx[i] = p[0] / p[3];
                    ny[i] = p[1] / p[3];
                    nz[i] = p[2] / p[3];
                }
                outGrid = clone;
#endif
                if (outGrid) {
                    outGrid->setTransform(vistle::Matrix4::Identity());
                } else {
                    sendError("Could not apply coordinate transformation");
                }
            }
            updateMeta(outGrid);
            m_gridCache.storeAndUnlock(gridEntry, outGrid);
        }
        if (split.mapped) {
            auto mapping = split.mapped->guessMapping();
            if (mapping != DataBase::Vertex) {
                sendError("data has to be mapped per vertex");
                out = outGrid;
            } else {
                auto data = split.mapped->clone();
                data->setGrid(outGrid);
                updateMeta(data);
                out = data;
            }
        } else {
            out = outGrid;
        }
        m_resultCache.storeAndUnlock(resultEntry, out);
    }

    addObject("grid_out", out);

    return true;
}
