#include "MapDrape.h"
#include <vistle/core/structuredgrid.h>
#include <vistle/core/object.h>
#include <proj_api.h>

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(PermutationOption, (XYZ)(XZY)(YXZ)(YZX)(ZXY)(ZYX))
MODULE_MAIN(MapDrape)

namespace {

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

PermutationOption invert(PermutationOption perm)
{
    switch (perm) {
    case YZX: {
        return ZXY;
        break;
    }
    case ZXY: {
        return YZX;
        break;
    }
    default: {
        return perm;
    }
    }
}

} // namespace

MapDrape::MapDrape(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    for (unsigned i = 0; i < NumPorts; ++i) {
        data_in[i] = createInputPort("data_in" + std::to_string(i));
        data_out[i] = createOutputPort("data_out" + std::to_string(i));
    }

    p_mapping_from_ = addStringParameter(
        "from", "PROJ6 order of coordinates for EPSG geo coordinate ref system: 1. Latitude, 2. Longitude",
        "+proj=latlong +datum=WGS84");
    p_mapping_to_ = addStringParameter("to", "", "+proj=tmerc +lat_0=0 +lon_0=9 +k=1.000000 +x_0=3500000 +y_0=0");

    p_offset = addVectorParameter("offset", "", ParamVector(0, 0, 0));

    p_permutation = addIntParameter("Axis Permutation", "permutation of the axis", XYZ, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_permutation, PermutationOption);
}

MapDrape::~MapDrape()
{}

bool MapDrape::compute()
{
    for (unsigned port = 0; port < NumPorts; ++port) {
        Coords::ptr outGeo;

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

        if (!isConnected(*data_out[port]))
            continue;

        StructuredGridBase::const_ptr inStr;
        const Scalar *xc = nullptr, *yc = nullptr, *zc = nullptr;
        auto it = m_alreadyMapped.find(geo->getName());
        if (it != m_alreadyMapped.end()) {
            outGeo = it->second;
        } else if (auto inCoords = Coords::as(geo)) {
            xc = &inCoords->x()[0];
            yc = &inCoords->y()[0];
            zc = &inCoords->z()[0];

            outGeo = inCoords->clone();
            updateMeta(outGeo);
            outGeo->resetCoords();
            outGeo->x().resize(inCoords->getNumCoords());
            outGeo->y().resize(inCoords->getNumCoords());
            outGeo->z().resize(inCoords->getNumCoords());

            assert(outGeo->getSize() == inCoords->getSize());
        } else if (inStr = StructuredGridBase::as(geo)) {
            Index dim[] = {inStr->getNumDivisions(0), inStr->getNumDivisions(1), inStr->getNumDivisions(2)};
            outGeo.reset(new StructuredGrid(dim[0], dim[1], dim[2]));
            outGeo->copyAttributes(geo);
            updateMeta(outGeo);
        } else {
            sendError("input geometry on port %s does neither have coordinates nor is it a structured grid",
                      data_in[port]->getName().c_str());
            return true;
        }

        if (inStr || (xc && yc && zc)) {
            assert(outGeo);
            auto perm = PermutationOption(p_permutation->getValue());
            permutate(perm, xc, yc, zc);

            auto xout = outGeo->x().data();
            auto yout = outGeo->y().data();
            auto zout = outGeo->z().data();

            projPJ pj_from = pj_init_plus(p_mapping_from_->getValue().c_str());
            projPJ pj_to = pj_init_plus(p_mapping_to_->getValue().c_str());
            if (!pj_from || !pj_to)
                return false;

            offset[0] = p_offset->getValue()[0];
            offset[1] = p_offset->getValue()[1];
            offset[2] = p_offset->getValue()[2];

            Index numCoords = outGeo->getSize();

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

            m_alreadyMapped[geo->getName()] = outGeo;
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
    m_alreadyMapped.clear();
    return true;
}

bool MapDrape::reduce(int timestep)
{
    if (timestep == -1)
        m_alreadyMapped.clear();
    return true;
}
