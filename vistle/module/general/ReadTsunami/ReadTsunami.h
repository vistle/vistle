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

#include <vistle/module/reader.h>
#include <vistle/core/polygons.h>

#include <netcdf>
#include <vector>
#include <array>

constexpr int NUM_BLOCKS{2};
constexpr int NUM_SCALARS{2};

namespace {

template<class UNumType>
struct Dim {
    UNumType dimLat;
    UNumType dimLon;

    Dim(const UNumType &lat, const UNumType &lon): dimLat(lat), dimLon(lon) {}
};

template<class UNumType>
struct PolygonData {
    UNumType numElements;
    UNumType numCorners;
    UNumType numVertices;

    PolygonData(const UNumType &elem, const UNumType &corn, const UNumType &vert)
    : numElements(elem), numCorners(corn), numVertices(vert)
    {}
};

template<class UNumType = size_t, class UNumTypeStep = std::ptrdiff_t>
struct NcVarParams {
    UNumType start;
    UNumType count;
    UNumTypeStep stride;
    UNumTypeStep imap;
    NcVarParams(const UNumType &start = 0, const UNumType &count = 0, const UNumTypeStep &stride = 1,
                const UNumTypeStep &imap = 1)
    : start(start), count(count), stride(stride), imap(imap)
    {}
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
    void initScalarParamReader();
    bool openNcFile(netCDF::NcFile &file) const;
    bool inspectNetCDFVars();

    template<class T, class U>
    using NcVarContainer = std::vector<NcVarParams<T, U>>;
    template<class T, class U, class S>
    void readNcVar(S *storage, const NcVarContainer<T, U> &ncParams, const netCDF::NcVar &nc);

    typedef std::function<float(size_t, size_t)> zCalcFunc;
    template<class U, class T, class V>
    vistle::Polygons::ptr generateSurface(
        const PolygonData<U> &polyData, const Dim<T> &dim, const std::vector<V> &coords,
        const zCalcFunc &func = [](size_t x, size_t y) { return 0; });

    template<class T, class U>
    bool computeBlock(Token &token, const T &blockNum, const U &timestep);

    template<class Iter>
    void computeBlockPartion(const int blockNum, size_t &ghost, vistle::Index &nLatBlocks, vistle::Index &nLonBlocks,
                             Iter blockPartitionIterFirst);

    template<class T>
    bool computeInitial(Token &token, const T &blockNum);

    template<class T, class U>
    bool computeTimestep(Token &token, const T &blockNum, const U &timestep);
    void computeActualLastTimestep(const ptrdiff_t &incrementTimestep, const size_t &firstTimestep,
                                   size_t &lastTimestep, size_t &nTimesteps);

    template<class T, class PartionIdx>
    NcVarParams<T> generateNcVarParams(const T &dim, const T &ghost, const T &numDimBlocks,
                                       const PartionIdx &partition) const;

    template<class T, class V>
    void fillCoordsPoly2Dim(vistle::Polygons::ptr poly, const Dim<T> &dim, const std::vector<V> &coords,
                            const zCalcFunc &zCalc);

    template<class T>
    void fillConnectListPoly2Dim(vistle::Polygons::ptr poly, const Dim<T> &dim);

    template<class T>
    void fillPolyList(vistle::Polygons::ptr poly, const T &numCorner);

    void printMPIStats() const;
    void printThreadStats() const;

    //Parameter
    vistle::StringParameter *p_filedir = nullptr;
    vistle::StringParameter *p_bathy = nullptr;
    vistle::FloatParameter *p_verticalScale = nullptr;
    std::array<vistle::IntParameter *, NUM_BLOCKS> m_blocks{nullptr, nullptr};
    std::array<vistle::StringParameter *, NUM_SCALARS> m_scalars;

    //Ports
    vistle::Port *p_seaSurface_out = nullptr;
    vistle::Port *p_groundSurface_out = nullptr;
    std::array<vistle::Port *, NUM_SCALARS> m_scalarsOut;

    //Polygons
    vistle::Polygons::ptr ptr_sea;

    //Scalar
    std::array<vistle::Vec<vistle::Scalar>::ptr, NUM_SCALARS> ptr_Scalar;

    //helper variables
    size_t verticesSea;
    size_t m_actualLastTimestep;
    std::vector<float> vecEta;

    //lat = 0; lon = 1
    std::array<std::string, NUM_BLOCKS> m_latLon_Surface;
    std::array<std::string, NUM_BLOCKS> m_latLon_Ground;
};
#endif
