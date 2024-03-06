#ifndef _ReadDuisburg_H
#define _ReadDuisburg_H

#include <cstddef>
#include <vistle/module/reader.h>

#include <pnetcdf>
#include <mpi.h>

//#include <vistle/core/polygons.h>
#include <vistle/core/triangles.h>
#include "vistle/core/layergrid.h"


using namespace vistle;

class ReadDuisburg: public vistle::Reader {
public:
    ReadDuisburg(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadDuisburg() override;

private:
    bool prepareRead() override;
    bool read(Token &token, int timestep = -1, int block = -1) override;
    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;
   
    bool cellIsWater(const std::vector<double> &h, int i, int j,int dimX, int dimY) const;
    bool getDimensions(const PnetCDF::NcmpiFile &ncFile, int &dimX, int &dimY)const ;

    Object::ptr generateTriangleGrid(const PnetCDF::NcmpiFile &ncFile, int timestep, int block) const;
    // Object::ptr generateLayerGrid(const PnetCDF::NcmpiFile &ncFile, int timestep, int block) const;

    Port *m_gridOut = nullptr;
    StringParameter *m_gridFile;

    std::mutex mtxPartList;

};

#endif
