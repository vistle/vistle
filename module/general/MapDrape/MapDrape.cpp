#include "MapDrape.h"
#include <vistle/core/structuredgrid.h>
#include <vistle/core/object.h>
#include <boost/mpl/for_each.hpp>

#ifdef MAPDRAPE
#ifdef USE_PROJ_API
#include <proj_api.h>
#else
#include <proj.h>
#endif

DEFINE_ENUM_WITH_STRING_CONVERSIONS(PermutationOption, (XYZ)(XZY)(YXZ)(YZX)(ZXY)(ZYX))
#endif

#ifdef DISPLACE
DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceComponent, (X)(Y)(Z)(All))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceOperation, (Set)(Add)(Multiply))
#endif

MODULE_MAIN(MapDrape)

using namespace vistle;

namespace {
#ifdef MAPDRAPE
template<class D>
void permutate(enum PermutationOption perm, D &x, D &y, D &z)
{
    switch (perm) {
    case XYZ: {
        break;
    }
    case XZY: {
        std::swap(y, z);
        break;
    }
    case YXZ: {
        std::swap(x, y);
        break;
    }
    case YZX: {
        std::swap(x, y);
        std::swap(y, z);
        break;
    }
    case ZXY: {
        std::swap(x, z);
        std::swap(y, z);
        break;
    }
    case ZYX: {
        std::swap(x, z);
        break;
    }
    default: {
        assert("axis permutation not handled" == nullptr);
    }
    }
}
#endif

#ifdef DISPLACE

template<class Grid>
struct GetCoord;
template<>
struct GetCoord<Coords> {
    const Scalar *ix[3];
    GetCoord(typename Coords::const_ptr &in): ix{&in->x()[0], &in->y()[0], &in->z()[0]} {}
    Vector3 operator()(Index i) { return Vector3(ix[0][i], ix[1][i], ix[2][i]); }
};

template<>
struct GetCoord<StructuredGridBase> {
    typename StructuredGridBase::const_ptr &in;
    GetCoord(typename StructuredGridBase::const_ptr &in): in(in) {}
    Vector3 operator()(Index i) { return in->getVertex(i); }
};

template<class Grid, int Dim>
struct DisplaceImpl {
    typename Grid::const_ptr &grid;
    DataBase::const_ptr &database;
    Coords::ptr &out;
    DisplaceComponent comp;
    DisplaceOperation op;

    DisplaceImpl(typename Grid::const_ptr &grid, DataBase::const_ptr &database, Coords::ptr &out,
                 DisplaceComponent comp, DisplaceOperation op)
    : grid(grid), database(database), out(out), comp(comp), op(op)
    {}

    template<typename S>
    void operator()(S)
    {
        typedef Vec<S, Dim> Data;
        typename Data::const_ptr data(Data::as(database));
        if (!data) {
            std::cerr << "data is not " << Object::toString(Data::type()) << std::endl;
            return;
        }

        const Index n = data->getSize();
        Scalar *ox[] = {out->x().data(), out->y().data(), out->z().data()};
        GetCoord<Grid> coord(grid);

        for (Index i = 0; i < n; ++i) {
            Vector3 v = coord(i);
            if (Dim == 1) {
                S d = data->x()[i];
                if (comp == All) {
                    for (unsigned c = 0; c < 3; ++c) {
                        switch (op) {
                        case Set:
                            v[c] = d;
                            break;
                        case Add:
                            v[c] += d;
                            break;
                        case Multiply:
                            v[c] *= d;
                            break;
                        }
                    }
                } else {
                    switch (op) {
                    case Set:
                        v[comp] = d;
                        break;
                    case Add:
                        v[comp] += d;
                        break;
                    case Multiply:
                        v[comp] *= d;
                        break;
                    }
                }
            }

            if (Dim == 3) {
                Vector3 d(data->x()[i], data->y()[i], data->z()[i]);
                switch (op) {
                case Set:
                    v = d;
                    break;
                case Add:
                    v += d;
                    break;
                case Multiply:
                    v = v.cwiseProduct(d);
                    break;
                }
            }
            ox[0][i] = v[0];
            ox[1][i] = v[1];
            ox[2][i] = v[2];
        }
    }
};

bool displace(Object::const_ptr &src, DataBase::const_ptr &data, Coords::ptr &coords, DisplaceComponent comp,
              DisplaceOperation op)
{
    if (data->getSize() != coords->getSize())
        return false;
    if (auto scoords = Coords::as(src)) {
        if (scoords->getSize() != coords->getSize())
            return false;
        DisplaceImpl<Coords, 1> d1(scoords, data, coords, comp, op);
        boost::mpl::for_each<Scalars>(d1);
        DisplaceImpl<Coords, 3> d3(scoords, data, coords, comp, op);
        boost::mpl::for_each<Scalars>(d3);
        return true;
    } else if (auto sstr = StructuredGridBase::as(src)) {
        if (sstr->getNumVertices() != coords->getSize())
            return false;
        DisplaceImpl<StructuredGridBase, 1> d1(sstr, data, coords, comp, op);
        boost::mpl::for_each<Scalars>(d1);
        DisplaceImpl<StructuredGridBase, 3> d3(sstr, data, coords, comp, op);
        boost::mpl::for_each<Scalars>(d3);
        return true;
    }
    return false;
}
#endif

} // namespace

MapDrape::MapDrape(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    for (unsigned i = 0; i < NumPorts; ++i) {
        data_in[i] = createInputPort("data_in" + std::to_string(i));
        data_out[i] = createOutputPort("data_out" + std::to_string(i));
    }

#ifdef MAPDRAPE
    p_mapping_from_ = addStringParameter(
        "from", "PROJ6 order of coordinates for EPSG geo coordinate ref system: 1. Latitude, 2. Longitude",
        "+proj=latlong +datum=WGS84");
    p_mapping_to_ = addStringParameter("to", "", "+proj=tmerc +lat_0=0 +lon_0=9 +k=1.000000 +x_0=3500000 +y_0=0");

    p_offset = addVectorParameter("offset", "", ParamVector(0, 0, 0));

    p_permutation = addIntParameter("Axis Permutation", "permutation of the axis", XYZ, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_permutation, PermutationOption);

    addResultCache(m_alreadyMapped);
#endif
#ifdef DISPLACE
    p_component = addIntParameter("component", "component to displace for scalar input", Z, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_component, DisplaceComponent);
    p_operation = addIntParameter("operation", "displacement operation to apply to selected component or element-wise",
                                  Add, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_operation, DisplaceOperation);

#endif
}

MapDrape::~MapDrape()
{}

bool MapDrape::compute()
{
    Coords::ptr outCoords;
    Object::const_ptr outGeo;
    for (unsigned port = 0; port < NumPorts; ++port) {
        if (!isConnected(*data_in[port]))
            continue;

        auto obj = expect<Object>(data_in[port]);
        if (!obj) {
            sendError("invalid data on %s", data_in[port]->getName().c_str());
            return true;
        }

        Object::const_ptr geo = obj;
        auto data = DataBase::as(obj);
        if (data && data->grid()) {
            geo = data->grid();
        } else {
            data.reset();
        }

#ifdef MAPDRAPE
        if (!isConnected(*data_out[port]))
            continue;
#endif
#ifdef DISPLACE
        if (port > 0 && !isConnected(*data_out[port]))
            continue;
#endif

#ifdef DISPLACE
        if (port > 0 && !outCoords)
            continue;
#endif

        StructuredGridBase::const_ptr inStr;
        const Scalar *xc = nullptr, *yc = nullptr, *zc = nullptr;
#ifdef MAPDRAPE
        auto cacheEntry = m_alreadyMapped.getOrLock(geo->getName(), outCoords);
        if (!cacheEntry) {
        } else
#endif
#ifdef DISPLACE
            if (!data) {
            outGeo = geo;
        } else
#endif

#ifdef DISPLACE
            if (!outCoords) {
#endif
            if (auto inCoords = Coords::as(geo)) {
                xc = &inCoords->x()[0];
                yc = &inCoords->y()[0];
                zc = &inCoords->z()[0];

                outCoords = inCoords->clone();
                updateMeta(outCoords);
                outCoords->resetCoords();
                outCoords->x().resize(inCoords->getNumCoords());
                outCoords->y().resize(inCoords->getNumCoords());
                outCoords->z().resize(inCoords->getNumCoords());

                assert(outCoords->getSize() == inCoords->getSize());
            } else if ((inStr = StructuredGridBase::as(geo))) {
                Index dim[] = {inStr->getNumDivisions(0), inStr->getNumDivisions(1), inStr->getNumDivisions(2)};
                outCoords.reset(new StructuredGrid(dim[0], dim[1], dim[2]));
                outCoords->copyAttributes(geo);
                updateMeta(outCoords);
            } else {
                sendError("input geometry on port %s does neither have coordinates nor is it a structured grid",
                          data_in[port]->getName().c_str());
                return true;
            }
#ifdef DISPLACE
        }
#endif

        if (inStr || (xc && yc && zc)) {
            assert(outCoords);

#ifdef MAPDRAPE
            auto xout = outCoords->x().data();
            auto yout = outCoords->y().data();
            auto zout = outCoords->z().data();

            auto perm = PermutationOption(p_permutation->getValue());
            permutate(perm, xc, yc, zc);

            offset[0] = p_offset->getValue()[0];
            offset[1] = p_offset->getValue()[1];
            offset[2] = p_offset->getValue()[2];

            Index numCoords = outCoords->getSize();

#ifdef USE_PROJ_API
            projPJ pj_from = pj_init_plus(p_mapping_from_->getValue().c_str());
            if (!pj_from) {
                sendError("could not initialize from-mapping");
                return true;
            }
            projPJ pj_to = pj_init_plus(p_mapping_to_->getValue().c_str());
            if (!pj_to) {
                pj_free(pj_from);
                sendError("could not initialize to-mapping");
                return true;
            }

            for (Index i = 0; i < numCoords; ++i) {
                double x, y, z;
                if (xc) {
                    assert(xc && yc && zc);
                    x = xc[i] * DEG_TO_RAD;
                    y = yc[i] * DEG_TO_RAD;
                    z = zc[i];
                } else {
                    assert(inStr);
                    auto v = inStr->getVertex(i);
                    permutate(perm, v[0], v[1], v[2]);
                    x = v[0] * DEG_TO_RAD;
                    y = v[1] * DEG_TO_RAD;
                    z = v[2];
                }

                pj_transform(pj_from, pj_to, 1, 1, &x, &y, &z);

                xout[i] = x + offset[0];
                yout[i] = y + offset[1];
                zout[i] = z + offset[2];
            }

            pj_free(pj_from);
            pj_free(pj_to);
#else
            /* NOTE: the use of PROJ strings to describe CRS is strongly discouraged */
            /* in PROJ 6, as PROJ strings are a poor way of describing a CRS, and */
            /* more precise its geodetic datum. */
            /* Use of codes provided by authorities (such as "EPSG:4326", etc...) */
            /* or WKT strings will bring the full power of the "transformation */
            /* engine" used by PROJ to determine the best transformation(s) between */
            /* two CRS. */
            PJ *P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, p_mapping_from_->getValue().c_str(),
                                           p_mapping_to_->getValue().c_str(), nullptr);
            if (!P) {
                sendError("could not initialize requested mapping");
                return true;
            }
            {
                /* proj_normalize_for_visualization() ensures that the coordinate */
                /* order expected and returned by proj_trans() will be longitude, */
                /* latitude for geographic CRS, and easting, northing for projected */
                /* CRS. If instead of using PROJ strings as above, "EPSG:XXXX" codes */
                /* had been used, this might had been necessary. */
                PJ *P_for_GIS = proj_normalize_for_visualization(PJ_DEFAULT_CTX, P);
                if (!P_for_GIS) {
                    sendError("could not initialize normalization mapping");
                    proj_destroy(P);
                    return true;
                }
                proj_destroy(P);
                P = P_for_GIS;
            }

            PJ_COORD c;
            c.lpzt.t = HUGE_VAL;
            for (Index i = 0; i < numCoords; ++i) {
                if (xc) {
                    assert(xc && yc && zc);
                    c.lpzt.lam = xc[i];
                    c.lpzt.phi = yc[i];
                    c.lpzt.z = zc[i];
                } else {
                    assert(inStr);
                    auto v = inStr->getVertex(i);
                    permutate(perm, v[0], v[1], v[2]);
                    c.lpzt.lam = v[0];
                    c.lpzt.phi = v[1];
                    c.lpzt.z = v[2];
                }

                PJ_COORD c_out = proj_trans(P, PJ_FWD, c);

                xout[i] = c_out.xyz.x + offset[0];
                yout[i] = c_out.xyz.y + offset[1];
                zout[i] = c_out.xyz.z + offset[2];
            }

            proj_destroy(P);
#endif

            m_alreadyMapped.storeAndUnlock(cacheEntry, outCoords);
#endif

#ifdef DISPLACE
            if (!displace(geo, data, outCoords, DisplaceComponent(p_component->getValue()),
                          DisplaceOperation(p_operation->getValue()))) {
                outCoords.reset();
                sendInfo("computing displaced grid failed");
                return true;
            }
#endif
        }

        if (!outGeo) {
            outGeo = outCoords;
        }

        if (data) {
            auto dataOut = data->clone();
            dataOut->setGrid(outGeo);
            addObject(data_out[port], dataOut);
        } else {
            passThroughObject(data_out[port], outGeo);
        }
    }

    return true;
}

bool MapDrape::prepare()
{
#ifdef DISPLACE
    if (!isConnected(*data_in[0])) {
        sendError("input on first port is required for computing output grid");
    }
#endif
    return true;
}

bool MapDrape::reduce(int timestep)
{
    return true;
}
