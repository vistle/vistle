#include "MapDrape.h"
#include <core/structuredgrid.h>
#include <core/object.h>
#include <proj_api.h>

using namespace vistle;

MODULE_MAIN(MapDrape)

MapDrape::MapDrape(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("MapDrape", name, moduleID, comm) {

    for (unsigned i=0; i<NumPorts; ++i) {
        data_in[i] = createInputPort("data_in" + std::to_string(i));
        data_out[i] = createOutputPort("data_out" + std::to_string(i));
    }

   p_mapping_from_ = addStringParameter("from","","+proj=latlong +datum=WGS84");
   p_mapping_to_ = addStringParameter("to","","+proj=tmerc +lat_0=0 +lon_0=9 +k=1.000000 +x_0=3500000 +y_0=0");

   p_offset = addVectorParameter("offset","",ParamVector(0,0,0));
}

MapDrape::~MapDrape() {
}

bool MapDrape::compute() {

    Coords::const_ptr inGeo;
    Coords::ptr outGeo;

    for (unsigned port=0; port<NumPorts; ++port) {
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

        auto coords = Coords::as(geo);
        if (!coords) {
            sendError("input geometry does not have coordinates");
            return true;
        }

        if (!isConnected(*data_out[port]))
            continue;

        if (inGeo) {
            if (inGeo != coords) {
                sendError("input geometry not identical across ports");
                return true;
            }
        } else {
            inGeo = coords;


            auto xc = &inGeo->x()[0];
            auto yc = &inGeo->y()[0];
            auto zc = &inGeo->z()[0];

            outGeo = coords->clone();
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

            int numCoords =  inGeo->getSize();

            for (int i = 0; i < numCoords; ++i) {
                double x = xc[i] * DEG_TO_RAD;
                double y = yc[i] * DEG_TO_RAD;
                double z = zc[i] ;

                pj_transform(pj_from, pj_to,1,1,&x,&y,&z);

                xout[i] = x + offset[0];
                yout[i] = y + offset[1];
                zout[i] = z + offset[2];
            }
        }

        if (data) {
            auto dataOut = data->clone();
            dataOut->setGrid(outGeo);
            addObject(data_out[port], dataOut);
        } else {
            addObject(data_out[port], outGeo);
        }
    }

    return true;
}



