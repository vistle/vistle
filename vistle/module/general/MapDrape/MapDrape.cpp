#include "MapDrape.h"
#include <vistle/core/structuredgrid.h>
#include <vistle/core/object.h>
#include <proj_api.h>

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(PermutationOption, (XYZ)(XZY)(YXZ)(YZX)(ZXY)(ZYX))
MODULE_MAIN(MapDrape)

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

        auto inGeo = Coords::as(geo);
        if (!inGeo) {
            sendError("input geometry on port %s does not have coordinates", data_in[port]->getName().c_str());
            return true;
        }

        if (!isConnected(*data_out[port]))
            continue;

        auto it = m_alreadyMapped.find(inGeo->getName());
        if (it == m_alreadyMapped.end()) {
            const Scalar *xc, *yc, *zc;
            switch (p_permutation->getValue()) {
            case XYZ: {
                xc = &inGeo->x()[0];
                yc = &inGeo->y()[0];
                zc = &inGeo->z()[0];
                break;
            }
            case XZY: {
                xc = &inGeo->x()[0];
                yc = &inGeo->z()[0];
                zc = &inGeo->y()[0];
                break;
            }
            case YXZ: {
                xc = &inGeo->y()[0];
                yc = &inGeo->x()[0];
                zc = &inGeo->z()[0];
                break;
            }
            case YZX: {
                xc = &inGeo->y()[0];
                yc = &inGeo->z()[0];
                zc = &inGeo->x()[0];
                break;
            }
            case ZXY: {
                xc = &inGeo->z()[0];
                yc = &inGeo->x()[0];
                zc = &inGeo->y()[0];
                break;
            }
            case ZYX: {
                xc = &inGeo->z()[0];
                yc = &inGeo->y()[0];
                zc = &inGeo->x()[0];
                break;
            }
            default: {
                assert("axis permutation not handled" == nullptr);
                xc = yc = zc = nullptr;
                return false;
            }
            }


            outGeo = inGeo->clone();
            updateMeta(outGeo);
            outGeo->resetCoords();
            outGeo->x().resize(inGeo->getNumCoords());
            outGeo->y().resize(inGeo->getNumCoords());
            outGeo->z().resize(inGeo->getNumCoords());
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

            Index numCoords = inGeo->getSize();
            assert(outGeo->getSize() == inGeo->getSize());

            for (Index i = 0; i < numCoords; ++i) {
                double x = xc[i] * DEG_TO_RAD;
                double y = yc[i] * DEG_TO_RAD;
                double z = zc[i];

                pj_transform(pj_from, pj_to, 1, 1, &x, &y, &z);

                xout[i] = x + offset[0];
                yout[i] = y + offset[1];
                zout[i] = z + offset[2];
            }

            m_alreadyMapped[inGeo->getName()] = outGeo;
        } else {
            outGeo = it->second;
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
