#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/lines.h>
#include <vistle/core/tubes.h>
#include <vistle/util/math.h>

#include "Tubes.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(MapMode, (Fixed)(Radius)(CrossSection)(InvRadius)(InvCrossSection))

MODULE_MAIN(ToTubes)

using namespace vistle;

ToTubes::ToTubes(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("grid_in");
    createInputPort("data_in");
    createOutputPort("grid_out");
    createOutputPort("data_out");

    m_radius = addFloatParameter("radius", "radius or radius scale factor of tube", 1.);
    setParameterMinimum(m_radius, (Float)0.);
    m_mapMode = addIntParameter("map_mode", "mapping of data to tube diameter", (Integer)Fixed, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_mapMode, MapMode);
    m_range = addVectorParameter("range", "allowed radius range", ParamVector(0., 1.));
    setParameterMinimum(m_range, ParamVector(0., 0.));

    m_startStyle =
        addIntParameter("start_style", "cap style for initial segments", (Integer)Tubes::Open, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_startStyle, CapStyle, Tubes);
    m_jointStyle = addIntParameter("connection_style", "cap style for segment connections", (Integer)Tubes::Round,
                                   Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_jointStyle, CapStyle, Tubes);
    m_endStyle = addIntParameter("end_style", "cap style for final segments", (Integer)Tubes::Open, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_endStyle, CapStyle, Tubes);
}

ToTubes::~ToTubes()
{}

bool ToTubes::compute()
{
    auto lines = accept<Lines>("grid_in");
    auto radius = accept<Vec<Scalar, 1>>("grid_in");
    auto radius3 = accept<Vec<Scalar, 3>>("grid_in");
    DataBase::ptr basedata;
    if (!lines && radius) {
        lines = Lines::as(radius->grid());
        basedata = radius->clone();
    }
    if (!lines && radius3) {
        lines = Lines::as(radius3->grid());
        basedata = radius3->clone();
    }
    if (!lines) {
        sendError("no Lines object");
        return true;
    }

    const MapMode mode = (MapMode)m_mapMode->getValue();
    if (mode != Fixed && !radius && !radius3) {
        sendError("data input required for varying radius");
        return true;
    }

    Tubes::ptr tubes;
    const Index *cl = nullptr;
    if (lines->getNumCorners() > 0)
        cl = &lines->cl()[0];
    // set coordinates
    if (lines->getNumCorners() == 0) {
        tubes = Tubes::clone<Vec<Scalar, 3>>(lines);
    } else {
        tubes.reset(new Tubes(lines->getNumElements(), lines->getNumCorners()));
        auto lx = &lines->x()[0];
        auto ly = &lines->y()[0];
        auto lz = &lines->z()[0];
        auto tx = tubes->x().data();
        auto ty = tubes->y().data();
        auto tz = tubes->z().data();
        for (Index i = 0; i < lines->getNumCorners(); ++i) {
            Index l = i;
            if (cl)
                l = cl[l];
            tx[i] = lx[l];
            ty[i] = ly[l];
            tz[i] = lz[l];
        }
    }

    // set radii
    auto r = tubes->r().data();
    auto radx = radius3 ? &radius3->x()[0] : radius ? &radius->x()[0] : nullptr;
    auto rady = radius3 ? &radius3->y()[0] : nullptr;
    auto radz = radius3 ? &radius3->z()[0] : nullptr;
    const Scalar scale = m_radius->getValue();
    const Scalar rmin = m_range->getValue()[0];
    const Scalar rmax = m_range->getValue()[1];
    for (Index i = 0; i < lines->getNumCorners(); ++i) {
        Index l = i;
        if (cl)
            l = cl[l];
        const Scalar rad = (radx && rady && radz) ? Vector3(radx[l], rady[l], radz[l]).norm()
                           : radx                 ? radx[l]
                                                  : Scalar(1.);
        switch (mode) {
        case Fixed:
            r[i] = scale;
            break;
        case Radius:
            r[i] = scale * rad;
            break;
        case CrossSection:
            r[i] = scale * sqrt(rad);
            break;
        case InvRadius:
            if (fabs(rad) >= 1e-9)
                r[i] = scale / rad;
            else
                r[i] = 1e9;
            break;
        case InvCrossSection:
            if (fabs(rad) >= 1e-9)
                r[i] = scale / sqrt(rad);
            else
                r[i] = 1e9;
            break;
        }

        r[i] = clamp(r[i], rmin, rmax);
    }

    // set tube lengths
    tubes->d()->components = lines->d()->el;

    tubes->setCapStyles((Tubes::CapStyle)m_startStyle->getValue(), (Tubes::CapStyle)m_jointStyle->getValue(),
                        (Tubes::CapStyle)m_endStyle->getValue());
    tubes->setMeta(lines->meta());
    tubes->copyAttributes(lines);
    updateMeta(tubes);

    if (basedata) {
        basedata->setGrid(tubes);
        updateMeta(basedata);
        addObject("grid_out", basedata);
    } else {
        addObject("grid_out", tubes);
    }

    auto data = accept<DataBase>("data_in");
    if (data) {
        auto ndata = data->clone();
        ndata->setGrid(tubes);
        updateMeta(ndata);
        addObject("data_out", ndata);
    }

    return true;
}
