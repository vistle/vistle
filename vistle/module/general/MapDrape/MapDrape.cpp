#include "MapDrape.h"
#include <core/structuredgrid.h>
#include <core/object.h>
#include <proj_api.h>

using namespace vistle;

MODULE_MAIN(MapDrape)

MapDrape::MapDrape(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("MapDrape", name, moduleID, comm) {

   geo_in = createInputPort("geo_in");
   data_out = createOutputPort("data_out");

   p_mapping_from_ = addStringParameter("from","","+proj=latlong +datum=WGS84");
   p_mapping_to_ = addStringParameter("to","","+proj=tmerc +lat_0=0 +lon_0=9 +k=1.000000 +x_0=3500000 +y_0=0");

   p_offset = addVectorParameter("offset","",ParamVector(0,0,0));
}

MapDrape::~MapDrape() {
}

bool MapDrape::compute() {
    auto obj = expect<Object>("geo_in");
    if (!obj)
        return true;

    Object::const_ptr geo = obj;
    auto data = DataBase::as(obj);
    if (data && data->grid()) {
        geo = data->grid();
    } else {
        data.reset();
    }

    auto coords = Coords::as(geo);
    if (!coords)
          return true;

    Coords::ptr outGeo = coords->clone();

    auto xc = outGeo->x().data();
    auto yc = outGeo->y().data();
    auto zc = outGeo->z().data();

    projPJ pj_from = pj_init_plus(p_mapping_from_->getValue().c_str());
    projPJ pj_to = pj_init_plus(p_mapping_to_->getValue().c_str());
    if (!pj_from || !pj_to)
        return false;

    offset[0] = p_offset->getValue()[0];
    offset[1] = p_offset->getValue()[1];
    offset[2] = p_offset->getValue()[2];

    int numCoords =  outGeo->getSize();

    for (int i = 0; i < numCoords; ++i) {
        double x = xc[i] * DEG_TO_RAD;
        double y = yc[i] * DEG_TO_RAD;
        double z = zc[i] ;

        pj_transform(pj_from, pj_to,1,1,&x,&y,&z);

        xc[i] = x + offset[0];
        yc[i] = y + offset[1];
        zc[i] = z + offset[2];
    }


    if (data){
        auto dataOut = data->clone();
        dataOut->setGrid(outGeo);
        addObject(data_out,dataOut);
    }else {
        addObject(data_out,outGeo);
    }
    return true;

}



