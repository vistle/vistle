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
#include <vector>
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

namespace {

template<class UNumType>
struct Dim {
    UNumType dimLat;
    UNumType dimLon;

    Dim(UNumType lat, UNumType lon): dimLat(lat), dimLon(lon) {}
};

template<class UNumType>
struct PolygonData {
    UNumType numElements;
    UNumType numCorners;
    UNumType numVertices;

    PolygonData(UNumType elem, UNumType corn, UNumType vert): numElements(elem), numCorners(corn), numVertices(vert) {}
};

} // namespace

class ReadTsunami: public vistle::Reader {
public:
    //default constructor
    ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadTsunami() override;

private:
    //Vistle functions
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;

    //Own functions
    bool openNcFile();

    typedef std::function<float(size_t, size_t)> zCalcFunc;
    template<class U, class T, class V>
    vistle::Polygons::ptr generateSurface(
        const PolygonData<U> &polyData, const Dim<T> &dim, const std::vector<V> &coords,
        const zCalcFunc &func = [](size_t x, size_t y) { return 0; });

    template<class T, class U>
    bool computeBlock(Token &token, const T &blockNum, const U &timestep);

    template<class T>
    bool computeInitialPolygon(Token &token, const T &blockNum);

    template<class T, class U>
    bool computeTimestepPolygon(Token &token, const T &blockNum, const U &timestep);

    //void functions
    template<class T, class V>
    void fillCoordsPoly2Dim(vistle::Polygons::ptr poly, const Dim<T> &dim, const std::vector<V> &coords,
                            const zCalcFunc &zCalc);

    template<class T>
    void fillConnectListPoly2Dim(vistle::Polygons::ptr poly, const Dim<T> &dim);

    template<class T>
    void fillPolyList(vistle::Polygons::ptr poly, const T &numCorner);

    void printMPIStats();
    void printThreadStats();

    //Parameter
    vistle::StringParameter *p_filedir = nullptr;
    vistle::FloatParameter *p_verticalScale = nullptr;
    vistle::IntParameter *p_ghostLayerWidth = nullptr;
    std::array<vistle::IntParameter *, 2> m_blocks;

    //Ports
    vistle::Port *p_seaSurface_out = nullptr;
    vistle::Port *p_groundSurface_out = nullptr;

    //Polygons
    vistle::Polygons::ptr ptr_sea;
    vistle::Polygons::ptr ptr_ground;

    //netCDF file to be read
    netCDF::NcFile m_ncDataFile;

    //helper variables
    float zScale;
    size_t countLatSea;
    size_t gridPolygons;
    size_t countLonSea;
    std::vector<float> vecEta;
};
#endif
