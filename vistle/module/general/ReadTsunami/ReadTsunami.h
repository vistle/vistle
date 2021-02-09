/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module for ChEESE tsunami nc-files         	       **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:    Marko Djuric                                                **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Date:  25.01.2021                                                      **
\**************************************************************************/
#ifndef _READTSUNAMI_H
#define _READTSUNAMI_H

#include "vistle/core/parameter.h"
#include <memory>
#include <vistle/module/reader.h>
#include <vistle/core/polygons.h>

#ifdef OLD_NETCDFCXX
#include <netcdfcpp>
#else
#include <ncFile.h>
#include <ncVar.h>
#include <ncDim.h>

#endif

//std
#include <cstddef>
#include <tuple>

#define NUMPARAMS 6

class ReadTsunami: public vistle::Reader {
public:
    //default constructor
    ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm);
    virtual ~ReadTsunami() override;

private:
    //Vistle functions
    /* bool prepareRead() override; */
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;
    /* bool finishRead() override; */

    //Own functions
    void initTupList();
    bool openNcFile();
    bool initNcData();
    bool checkValidNcVar();
    vistle::Polygons::ptr generateSurface(const size_t &numElem, const size_t &numCorner, const size_t &numVertices,
                                          const std::vector<size_t> &dimension, const std::vector<float *> &coords);

    void fillCoords2DimPoly(vistle::Polygons::ptr poly, const size_t &dimX, const size_t &dimY,
                            const std::vector<float *> &coords);
    void fillCoords3DimPoly(vistle::Polygons::ptr poly, const size_t &dimX, const size_t &dimY, const size_t &dimZ,
                            const std::vector<float *> &coords);
    void fillConnectList2DimPoly(vistle::Polygons::ptr poly, const size_t &dimX, const size_t &dimY);
    void fillPolyList(vistle::Polygons::ptr poly, const size_t &numElem, const size_t &numCorner);
    void block(Token &token, vistle::Index bx, vistle::Index by, vistle::Index bz, vistle::Index b,
               vistle::Index time) const;
    void initHelperVariables();

    //Parameter
    vistle::StringParameter *p_filedir = nullptr;
    vistle::FloatParameter *p_verticalScale = nullptr;
    vistle::IntParameter *p_step = nullptr;
    /* vistle::IntParameter *m_blocks[3]; */
    vistle::IntParameter *p_ghostLayerWidth = nullptr;
    /* vistle::IntParameter *m_size[3]; */

    //Ports
    vistle::Port *p_seaSurface_out = nullptr;
    vistle::Port *p_groundSurface_out = nullptr;
    vistle::Port *p_maxHeight = nullptr;

    //Polygons
    vistle::Polygons::ptr ptr_sea;
    vistle::Polygons::ptr ptr_ground;

    //netCDF file to be read
    netCDF::NcFile *p_ncDataFile;

    //netCDF data objects
    netCDF::NcVar latvar;
    netCDF::NcVar lonvar;
    netCDF::NcVar grid_latvar;
    netCDF::NcVar grid_lonvar;
    netCDF::NcVar bathymetryvar;
    netCDF::NcVar max_height;
    netCDF::NcVar eta;

    //helper pointers and variables
    float *p_scalarMaxHeight;
    float *p_eta;
    float zScale;
    size_t surfaceDimX;
    size_t surfaceDimY;
    size_t surfaceDimZ;
    size_t gridLatDimX;
    size_t gridLonDimY;
    size_t numPolygons;

    //netCDF paramlist
    std::vector<netCDF::NcVar *> vec_NcVar;
};

inline float zCalcGround(float *zValues, size_t x_it, size_t y_it, size_t gridLon)
{
    return -zValues[x_it * gridLon + y_it];
}

inline float zCalcSeaHeight(float *zValues, size_t nx, size_t ny, float zScale, int time, int n)
{
    return zValues[time * nx * ny + n] * zScale;
}

#endif
