#include "objalg.h"

#include <vistle/core/placeholder.h>
#include <vistle/core/coords.h>
#include <vistle/core/layergrid.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/database.h>

namespace vistle {

#define GETMETA \
    if (data) { \
        split.block = data->getBlock(); \
    } \
    if (split.block == -1 && grid) { \
        split.block = grid->getBlock(); \
    } \
    if (split.block == -1 && normals) { \
        split.block = normals->getBlock(); \
    } \
\
    if (data) { \
        split.timestep = data->getTimestep(); \
    } \
    if (split.timestep == -1 && grid) { \
        split.timestep = grid->getTimestep(); \
    } \
    if (split.timestep == -1 && normals) { \
        split.timestep = normals->getTimestep(); \
    } \
\
    if (data) { \
        split.iteration = data->getIteration(); \
    } \
    if (split.iteration == -1 && grid) { \
        split.iteration = grid->getIteration(); \
    } \
    if (split.iteration == -1 && normals) { \
        split.iteration = normals->getIteration(); \
    }

DataComponents splitContainerObject(Object::const_ptr container)
{
    DataComponents split;
    auto &grid = split.geometry;
    auto &normals = split.normals;
    auto &data = split.mapped;

    if (auto ph = vistle::PlaceHolder::as(container)) {
        grid = ph->geometry();
        auto normals = ph->normals();
        auto data = ph->texture();
        GETMETA;
        return split;
    } else {
        if (auto c = vistle::Coords::as(container)) {
            // do not treat as Vec<Scalar,3>
            grid = c;
        } else if (auto l = vistle::LayerGrid::as(container)) {
            // do not treat as Vec<Scalar,1>
            grid = l;
        } else if (auto d = vistle::DataBase::as(container)) {
            data = d;
            grid = d->grid();
        } else {
            grid = container;
        }
        if (grid) {
            if (auto gi = grid->getInterface<GeometryInterface>()) {
                normals = gi->normals();
            }
        }
    }

    GETMETA;

    return split;
}

} // namespace vistle
