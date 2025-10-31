#include <sstream>
#include <iomanip>

#include <boost/mpl/for_each.hpp>

#include <vistle/core/object.h>
#include <vistle/core/indexed.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/celltypes.h>
#include <vistle/alg/objalg.h>

#include "ManipulateGhosts.h"

MODULE_MAIN(ManipulateGhosts)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Operation, (Identity)(Invert)(Set)(Unset)(Remove));

ManipulateGhosts::ManipulateGhosts(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    m_gridIn = createInputPort("grid_in", "unstructured grid");
    m_gridOut = createOutputPort("grid_out", "unstructured grid with tesselated polyhedra");

    m_operation = addIntParameter("operation", "operation to perform", Identity, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_operation, Operation);
}

namespace {
template<class Geo>
typename Geo::ptr applyGhostOp(const typename Geo::const_ptr &geo, Operation op)
{
    auto nelem = geo->getNumElements();
    typename Geo::ptr ret = geo->clone();
    if (op == Identity) {
        return ret;
    }
    if (op == Remove) {
        ret->d()->ghost = ShmVector<Byte>();
        ret->d()->ghost.construct();
        return ret;
    }

    ret->d()->ghost = ShmVector<Byte>();
    ret->d()->ghost.construct(nelem);
    Byte *out = ret->d()->ghost->data();

    const Byte *in = nullptr;
    if (geo->ghost().size() > 0) {
        in = geo->ghost().data();
    }

    for (Index elem = 0; elem < nelem; ++elem) {
        Byte tin = in ? in[elem] : cell::NORMAL;
        auto &tout = out[elem];
        switch (op) {
        case Invert:
            tout = tin == cell::NORMAL ? cell::GHOST : cell::NORMAL;
            break;
        case Set:
            tout = cell::GHOST;
            break;
        case Unset:
            tout = cell::NORMAL;
            break;
        case Remove:
        case Identity:
            break;
        }
    }

    return ret;
}
} // namespace

bool ManipulateGhosts::compute()
{
    auto obj = expect<Object>(m_gridIn);
    if (!obj) {
        sendError("no input");
        return true;
    }

    auto split = splitContainerObject(obj);
    auto geo = split.geometry;
    if (!geo) {
        sendError("input does not have geometry");
        return true;
    }

    Object::ptr outgeo;
    if (auto tri = Triangles::as(geo)) {
        outgeo = applyGhostOp<Triangles>(tri, Operation(m_operation->getValue()));
    } else if (auto quad = Quads::as(geo)) {
        outgeo = applyGhostOp<Quads>(quad, Operation(m_operation->getValue()));
    } else if (auto ind = Indexed::as(geo)) {
        outgeo = applyGhostOp<Indexed>(ind, Operation(m_operation->getValue()));
    } else {
        sendError("input geometry type not supported");
        return true;
    }

    updateMeta(outgeo);

    if (split.mapped) {
        auto mapped = split.mapped->clone();
        mapped->setGrid(outgeo);
        updateMeta(mapped);
        addObject(m_gridOut, mapped);
    } else {
        addObject(m_gridOut, outgeo);
    }

    return true;
}
