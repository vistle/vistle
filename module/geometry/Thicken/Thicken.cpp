#include <sstream>
#include <iomanip>

#include <boost/mpl/for_each.hpp>

#include <vistle/core/object.h>
#include <vistle/core/texture1d.h>
#include <vistle/core/coords.h>
#include <vistle/core/points.h>
#include <vistle/core/spheres.h>
#include <vistle/core/lines.h>
#include <vistle/core/tubes.h>
#include <vistle/util/math.h>
#include <vistle/alg/objalg.h>

#include "Thicken.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(MapMode,
                                    (DoNothing)(Fixed)(Radius)(InvRadius)(Surface)(InvSurface)(Volume)(InvVolume))

MODULE_MAIN(Thicken)

using namespace vistle;

Thicken::Thicken(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("grid_in", "lines or points with scalar data for radius");
    createInputPort("data_in", "mapped data");
    createOutputPort("grid_out", "tubes or spheres");
    createOutputPort("data_out", "tubes or spheres with mapped data");

    m_radius = addFloatParameter("radius", "radius or radius scale factor of tube/sphere", 1.);
    setParameterMinimum(m_radius, (Float)0.);
    m_sphereScale = addFloatParameter("sphere_scale", "extra scale factor for spheres", 2.5);
    setParameterMinimum(m_sphereScale, (Float)0.);
    m_mapMode = addIntParameter("map_mode", "mapping of data to tube diameter", (Integer)Fixed, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_mapMode, MapMode);
    m_range = addVectorParameter("range", "allowed radius range", ParamVector(0., 1.));
    setParameterMinimum(m_range, ParamVector(0., 0.));

    m_startStyle =
        addIntParameter("start_style", "cap style for initial tube segments", (Integer)Tubes::Open, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_startStyle, CapStyle, Tubes);
    m_jointStyle = addIntParameter("connection_style", "cap style for tube segment connections", (Integer)Tubes::Round,
                                   Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_jointStyle, CapStyle, Tubes);
    m_endStyle =
        addIntParameter("end_style", "cap style for final tube segments", (Integer)Tubes::Open, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_endStyle, CapStyle, Tubes);
}

Thicken::~Thicken()
{}

template<int Dim>
struct FlattenData {
    DataBase::const_ptr object;
    DataBase::ptr &result;
    Index num = 0;
    const Index *const cl = nullptr;
    FlattenData(DataBase::const_ptr obj, DataBase::ptr &result, Index num, const Index *cl)
    : object(obj), result(result), num(num), cl(cl)
    {
        assert(num == 0 || cl);
    }
    template<typename S>
    void operator()(S)
    {
        typedef Vec<S, Dim> V;
        typename V::const_ptr in(V::as(object));
        if (!in)
            return;

        typename V::ptr out = std::make_shared<V>(num);
        for (int d = 0; d < Dim; ++d) {
            const auto *din = &in->x(d)[0];
            auto *dout = out->x(d).data();

            for (Index i = 0; i < num; ++i) {
                *dout++ = din[cl[i]];
            }
        }
        result = out;
    }
};

DataBase::ptr flattenData(DataBase::const_ptr src, Index num, const Index *cl)
{
    assert(num == 0 || cl);
    if (num == 0) {
        return src->clone();
    }
    DataBase::ptr result;
    boost::mpl::for_each<Scalars>(FlattenData<1>(src, result, num, cl));
    boost::mpl::for_each<Scalars>(FlattenData<3>(src, result, num, cl));
    if (result)
        result->copyAttributes(src);
    if (auto tex = Texture1D::as(src)) {
        assert(result);
        auto vec1 = Vec<Scalar, 1>::as(Object::ptr(result));
        assert(vec1);
        auto result2 = tex->clone();
        result2->d()->x[0] = vec1->d()->x[0];
        result = result2;
    }
    return result;
}


bool Thicken::compute()
{
    auto updateOutput = [&](OutputGeometry og) {
        if (m_output != og) {
            m_output = og;
            switch (m_output) {
            case OGUnknown:
                setItemInfo("");
                break;
            case OGError:
                setItemInfo("Error");
                break;
            case OGSpheres:
                setItemInfo("Spheres");
                break;
            case OGTubes:
                setItemInfo("Tubes");
                break;
            }
        }
    };

    auto obj = expect<Object>("grid_in");
    if (!obj) {
        updateOutput(OGError);
        sendError("no input data");
        return true;
    }

    MapMode mode = (MapMode)m_mapMode->getValue();
    if (mode == DoNothing) {
        auto nobj = obj->clone();
        updateMeta(nobj);
        addObject("grid_out", nobj);

        auto data = accept<Object>("data_in");
        if (data) {
            if (auto d = DataBase::as(data)) {
                auto ndata = d->clone();
                if (d->grid() == obj) {
                    ndata->setGrid(nobj);
                }
                updateMeta(ndata);
                addObject("data_out", ndata);
            } else {
                auto ndata = data->clone();
                updateMeta(ndata);
                addObject("data_out", ndata);
            }
        }
        return true;
    }

    auto split = splitContainerObject(obj);
    auto lines = Lines::as(split.geometry);
    auto points = Points::as(split.geometry);
    if (!points && !lines) {
        sendError("no Lines and no Points object");
        updateOutput(OGError);
        return true;
    }

    auto &basedatain = split.mapped;
    auto radius = Vec<Scalar, 1>::as(split.mapped);
    auto radius3 = Vec<Scalar, 3>::as(split.mapped);
    auto iradius = Vec<Index>::as(split.mapped);

    DataBase::ptr basedata;
    if (basedatain) {
        if (basedatain->guessMapping() != DataBase::Vertex) {
            sendInfo("vertex mapping required for varying radius");
            updateOutput(OGError);
            return true;
        }
        if (basedatain->guessMapping() == DataBase::Vertex && isConnected("grid_out")) {
            basedata = flattenData(basedatain, lines ? lines->getNumCorners() : 0, lines ? &lines->cl()[0] : nullptr);
        }
    } else {
        if (mode != Fixed && !radius && !radius3 && !iradius) {
            sendInfo("data input required for varying radius");
        }
    }

    if (lines && mode == InvVolume) {
        sendInfo("setting inverse tube cross section for Lines input");
        mode = InvSurface;
    }

    if (lines && mode == Volume) {
        sendInfo("setting tube cross section for Lines input");
        mode = Surface;
    }

    Tubes::ptr tubes;
    Spheres::ptr spheres;
    CoordsWithRadius::ptr cwr;

    const Index *cl = nullptr;
    if (lines) {
        updateOutput(OGTubes);
        // set coordinates
        if (lines->getNumCorners() == 0) {
            tubes = Tubes::clone<Vec<Scalar, 3>>(lines);
        } else {
            cl = &lines->cl()[0];
            tubes.reset(new Tubes(lines->getNumElements(), lines->getNumCorners()));
            auto lx = &lines->x()[0];
            auto ly = &lines->y()[0];
            auto lz = &lines->z()[0];
            auto tx = tubes->x().data();
            auto ty = tubes->y().data();
            auto tz = tubes->z().data();
            for (Index i = 0; i < lines->getNumCorners(); ++i) {
                Index l = cl[i];
                tx[i] = lx[l];
                ty[i] = ly[l];
                tz[i] = lz[l];
            }
        }
        cwr = tubes;

        // set tube lengths
        tubes->d()->components = lines->d()->el;

        tubes->setCapStyles((Tubes::CapStyle)m_startStyle->getValue(), (Tubes::CapStyle)m_jointStyle->getValue(),
                            (Tubes::CapStyle)m_endStyle->getValue());
        tubes->setMeta(lines->meta());
        tubes->copyAttributes(lines);
    } else if (points) {
        updateOutput(OGSpheres);
        spheres = Spheres::clone<Vec<Scalar, 3>>(points);
        cwr = spheres;
    }

    assert(cwr);

    // set radii
    auto r = cwr->r().data();
    auto radx = radius3 ? &radius3->x()[0] : radius ? &radius->x()[0] : nullptr;
    auto rady = radius3 ? &radius3->y()[0] : nullptr;
    auto radz = radius3 ? &radius3->z()[0] : nullptr;
    auto irad = iradius ? &iradius->x()[0] : nullptr;
    const Scalar extraScale = spheres ? m_sphereScale->getValue() : 1.;
    const Scalar scale = m_radius->getValue() * extraScale;
    const Scalar rmin = m_range->getValue()[0] * extraScale;
    const Scalar rmax = m_range->getValue()[1] * extraScale;
    Index nv = lines ? lines->getNumCorners() : points->getNumPoints();
    for (Index i = 0; i < nv; ++i) {
        Index l = cl ? cl[i] : i;
        const Scalar rad = (radx && rady && radz) ? Vector3(radx[l], rady[l], radz[l]).norm()
                           : radx                 ? radx[l]
                           : irad                 ? Scalar(irad[l])
                                                  : Scalar(1.);
        switch (mode) {
        case DoNothing:
            break;
        case Fixed:
            r[i] = scale;
            break;
        case Radius:
            r[i] = scale * rad;
            break;
        case Surface:
            r[i] = scale * sqrt(rad);
            break;
        case InvRadius:
            if (fabs(rad) >= 1e-9)
                r[i] = scale / rad;
            else
                r[i] = 1e9;
            break;
        case InvSurface:
            if (fabs(rad) >= 1e-9)
                r[i] = scale / sqrt(rad);
            else
                r[i] = 1e9;
            break;
        case Volume:
            r[i] = scale * powf(rad, 0.333333f);
            break;
        case InvVolume:
            if (fabs(rad) >= 1e-9)
                r[i] = scale / powf(rad, 0.333333f);
            else
                r[i] = 1e9;
            break;
        }

        r[i] = clamp(r[i], rmin, rmax);
    }

    updateMeta(cwr);
    if (basedata) {
        basedata->setGrid(cwr);
        updateMeta(basedata);
        addObject("grid_out", basedata);
    } else {
        addObject("grid_out", cwr);
    }

    auto data = accept<DataBase>("data_in");
    if (data && isConnected("data_out")) {
        auto mapping = data->guessMapping();
        switch (mapping) {
        case DataBase::Vertex: {
            auto ndata = flattenData(data, lines ? lines->getNumCorners() : 0, lines ? &lines->cl()[0] : nullptr);
            ndata->setGrid(cwr);
            updateMeta(ndata);
            addObject("data_out", ndata);
            break;
        }
        case DataBase::Element: {
            auto ndata = data->clone();
            ndata->setGrid(cwr);
            updateMeta(ndata);
            addObject("data_out", ndata);
            break;
        }
        default: {
            sendError("unsupported data mapping");
            break;
        }
        }
    }

    return true;
}
