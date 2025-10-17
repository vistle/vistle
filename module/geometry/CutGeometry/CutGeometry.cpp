#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/polygons.h>
#include <vistle/core/triangles.h>
#include <vistle/util/math.h>
#include <vistle/alg/objalg.h>

#include "CutGeometry.h"
#include "PlaneClip.h"
#include "../../map/IsoSurface/IsoDataFunctor.h"

MODULE_MAIN(CutGeometry)

using namespace vistle;

CutGeometry::CutGeometry(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm), isocontrol(this)
{
    isocontrol.init();

    auto pin = createInputPort("data_in", "bare input grid without mapped data");
    auto pout = createOutputPort("data_out", "clipped grid");
    linkPorts(pin, pout);
}

CutGeometry::~CutGeometry()
{}

Object::ptr CutGeometry::cutGeometry(Object::const_ptr object) const
{
    auto coords = Coords::as(object);
    if (!coords)
        return Object::ptr();

    IsoDataFunctor decider =
        isocontrol.newFunc(object->getTransform(), coords->x().data(), coords->y().data(), coords->z().data());

    switch (object->getType()) {
    case Object::TRIANGLES: {
        PlaneClip cutter(Triangles::as(object), decider);
        cutter.process();
        return cutter.result();
    }

    case Object::POLYGONS: {
        PlaneClip cutter(Polygons::as(object), decider);
        cutter.process();
        return cutter.result();
    }

    default:
        break;
    }
    return Object::ptr();
}

bool CutGeometry::changeParameter(const Parameter *param)
{
    bool ok = isocontrol.changeParameter(param);
    return Module::changeParameter(param) && ok;
}

bool CutGeometry::compute(const std::shared_ptr<BlockTask> &task) const
{
    Object::const_ptr oin = task->expect<Object>("data_in");
    if (!oin)
        return true;

    auto split = splitContainerObject(oin);
    if (!split.geometry) {
        sendError("did not receive a geometry input object");
        return true;
    }

    Object::ptr object = cutGeometry(split.geometry);
    if (object) {
        object->copyAttributes(split.geometry);
        updateMeta(object);
        task->addObject("data_out", object);
    }

    return true;
}
