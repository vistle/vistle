#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/spheres.h>
#include <vistle/util/math.h>

#include "Spheres.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(MapMode, (Fixed)(Radius)(Surface)(Volume))

MODULE_MAIN(ToSpheres)

using namespace vistle;

ToSpheres::ToSpheres(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("grid_in", "points or scalar data on points");
    createInputPort("map_in", "mapped data");
    createOutputPort("grid_out", "spheres");

    auto MaxRad = std::numeric_limits<Scalar>::max();

    m_radius = addFloatParameter("radius", "radius or radius scale factor of spheres", 1.);
    setParameterMinimum(m_radius, (Float)0.);
    m_mapMode = addIntParameter("map_mode", "mapping of data to sphere size", (Integer)Fixed, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_mapMode, MapMode);
    m_range = addVectorParameter("range", "allowed radius range", ParamVector(0., MaxRad));
    setParameterMinimum(m_range, ParamVector(0., 0.));
}

ToSpheres::~ToSpheres()
{}

bool ToSpheres::compute()
{
    auto points = accept<Vec<Scalar, 3>>("grid_in");
    Vec<Scalar, 1>::const_ptr radius;
    Vec<Scalar, 3>::const_ptr radius3;
    if (points && points->grid()) {
        radius3 = points;
        points = Vec<Scalar, 3>::as(points->grid());
    } else {
        radius = accept<Vec<Scalar, 1>>("grid_in");
        if (radius) {
            points = Vec<Scalar, 3>::as(radius->grid());
        }
    }
    if (!points) {
        sendError("no Points object");
        return true;
    }

    auto data = accept<DataBase>("map_in");

    MapMode mode = (MapMode)m_mapMode->getValue();
    if (mode != Fixed && !radius && !radius3) {
        sendError("data input required for varying radius");
        return true;
    }

    Spheres::ptr spheres = Spheres::clone<Vec<Scalar, 3>>(points);
    auto r = spheres->r().data();

    std::vector<Scalar> rad3;
    auto rad = radius ? &radius->x()[0] : nullptr;
    if (!rad && radius3) {
        rad3.resize(radius3->getSize());
        auto x = &radius3->x()[0], y = &radius3->y()[0], z = &radius3->z()[0];
        for (Index i = 0; i < rad3.size(); ++i) {
            rad3[i] = Vector3(x[i], y[i], z[i]).norm();
        }
        rad = rad3.data();
    }
    if (!rad) {
        mode = Fixed;
    }
    const Scalar scale = m_radius->getValue();
    const Scalar rmin = m_range->getValue()[0];
    const Scalar rmax = m_range->getValue()[1];
    for (Index i = 0; i < spheres->getNumSpheres(); ++i) {
        switch (mode) {
        case Fixed:
            r[i] = scale;
            break;
        case Radius:
            r[i] = scale * rad[i];
            break;
        case Surface:
            r[i] = scale * sqrt(rad[i]);
            break;
        case Volume:
            r[i] = scale * powf(rad[i], 0.333333f);
            break;
        }

        r[i] = clamp(r[i], rmin, rmax);
    }
    spheres->setMeta(points->meta());
    spheres->copyAttributes(points);
    updateMeta(spheres);

    if (data) {
        auto dout = data->clone();
        dout->setGrid(spheres);
        updateMeta(dout);
        addObject("grid_out", dout);
    } else {
        addObject("grid_out", spheres);
    }

    return true;
}
