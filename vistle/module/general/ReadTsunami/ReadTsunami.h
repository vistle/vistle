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

#include "vistle/core/index.h"
#include "vistle/core/parameter.h"
#include <string>
#include <vistle/module/reader.h>
#include <vistle/core/polygons.h>

#ifdef OLD_NETCDFCXX
#include <netcdfcpp>
#else
#include <ncFile.h>
#include <ncVar.h>
#include <ncDim.h>

#endif

/* #define NUMPARAMS 6 */

class ReadTsunami: public vistle::Reader {
public:
    //default constructor
    ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm);
    /* virtual ~ReadTsunami() override; */

private:
    //Vistle functions
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;

    //Own functions
    void initNcVarVec();
    bool openNcFile();
    bool initNcData();
    bool checkValidNcVar();
    vistle::Polygons::ptr generateSurface(const size_t &numElem, const size_t &numCorner, const size_t &numVertices,
                                          const std::vector<float *> &coords);

    //void functions
    void initHelperVariables();
    void fillCoordsPoly2Dim(vistle::Polygons::ptr poly, const size_t &dimX, const size_t &dimY,
                            const std::vector<float *> &coords, const int &blockNum);
    void fillConnectListPoly2Dim(vistle::Polygons::ptr poly, const size_t &dimX, const size_t &dimY);
    void fillPolyList(vistle::Polygons::ptr poly, const size_t &numCorner);

    void computeBlock(Token &token, vistle::Index bx, vistle::Index by, vistle::Index bz, vistle::Index b,
                      vistle::Index time) const;
    void computeBlock(Token &token, vistle::Index b, vistle::Index time);

    //without block
    void computeInitialPolygon(Token &token);
    void computeTimestepPolygon(Token &token, const int &timestep);

    void computeInitialPolygon(Token &token, const vistle::Index &blockNum);
    void computeTimestepPolygon(Token &token, const vistle::Index &timestep, const vistle::Index &blockNum);

    //Parameter
    vistle::StringParameter *p_filedir = nullptr;
    vistle::FloatParameter *p_verticalScale = nullptr;
    vistle::IntParameter *m_blocks[3];
    vistle::IntParameter *p_ghostLayerWidth = nullptr;

    //Ports
    vistle::Port *p_seaSurface_out = nullptr;
    vistle::Port *p_groundSurface_out = nullptr;
    vistle::Port *p_maxHeight = nullptr;

    //Polygons
    vistle::Polygons::ptr ptr_sea;
    vistle::Polygons::ptr ptr_ground;

    //netCDF file to be read
    netCDF::NcFile m_ncDataFile;

    //netCDF data objects
    netCDF::NcVar latvar;
    netCDF::NcVar lonvar;
    netCDF::NcVar grid_latvar;
    netCDF::NcVar grid_lonvar;
    netCDF::NcVar bathymetryvar;
    netCDF::NcVar max_height;
    netCDF::NcVar eta;

    //helper variables
    float zScale;
    size_t surfaceDimX;
    size_t surfaceDimY;
    size_t surfaceDimZ;
    size_t gridLatDimX;
    size_t gridLonDimY;
    size_t gridPolygons;
    std::vector<float> vec_maxH;
    std::vector<float> vec_eta;

    //netCDF paramlist
    std::vector<netCDF::NcVar *> vec_NcVar;
};
#endif
