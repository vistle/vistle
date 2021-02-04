/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module for Tsunami data         	                   **
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

#include <vistle/module/reader.h>
#include <vistle/core/polygons.h>

#ifdef OLD_NETCDFCXX
#include <netcdfcpp>
#else
#include <ncFile.h>
#include <ncVar.h>
#include <ncDim.h>

/* using namespace netCDF; */
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
    bool finishRead() override;

    //Own functions
    bool openNcFile();
    bool initNcData();
    bool checkValidNcVar();
    vistle::Polygons::ptr generateSurface(const size_t &numElem, const size_t &numCorner, const size_t &numVertices,
                                          const std::vector<size_t> &dimension, const std::vector<float *> &coords);

    void initTupList();
    void fillCoords2DimPoly(vistle::Polygons::ptr poly, const size_t &dimX, const size_t &dimY,
                            const std::vector<float *> &coords);
    void fillCoords3DimPoly(vistle::Polygons::ptr poly, const size_t &dimX, const size_t &dimY, const size_t &dimZ,
                            const std::vector<float *> &coords);
    void fillConnectList2DimPoly(vistle::Polygons::ptr poly, const size_t &dimX, const size_t &dimY);
    void fillPolyList4Corner(vistle::Polygons::ptr poly, const size_t &numElem);
    void block(Token &token, vistle::Index bx, vistle::Index by, vistle::Index bz, vistle::Index b,
               vistle::Index time) const;

    //Parameter
    vistle::StringParameter *m_filedir;
    vistle::StringParameter *m_variables[NUMPARAMS];
    vistle::FloatParameter *m_verticalScale;
    vistle::IntParameter *m_step;
    vistle::IntParameter *m_blocks[3];
    vistle::IntParameter *m_ghostLayerWidth;
    vistle::IntParameter *m_size[3];

    //Ports
    vistle::Port *m_grid_out = nullptr;
    vistle::Port *m_dataOut[NUMPARAMS];
    vistle::Port *m_surface_out;
    vistle::Port *m_seaSurface_out;
    vistle::Port *m_waterSurface_out;
    vistle::Port *m_maxHeight;

    //Polygons
    vistle::Polygons::ptr sea;
    vistle::Polygons::ptr ground;

    //netCDF file to be read
    netCDF::NcFile *ncDataFile;

    //netCDF data objects
    netCDF::NcVar latvar;
    netCDF::NcVar lonvar;
    netCDF::NcVar grid_latvar;
    netCDF::NcVar grid_lonvar;
    netCDF::NcVar bathymetryvar;
    netCDF::NcVar max_height;
    netCDF::NcVar eta;

    //netCDF paramlist
    std::vector<netCDF::NcVar *> t_NcVar;
};

inline float zCalcGround(float *zValues, size_t x_it, size_t y_it, size_t gridLon)
{
    return -zValues[x_it * gridLon + y_it];
}

inline float zCalcSea(float *zValues, size_t x_it, size_t y_it, float zScale, int time, int it)
{
    return zValues[time * x_it * y_it + it] * zScale;
}

#endif
