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

#include <memory>
#include <mpi.h>
#include <mutex>
#include <vistle/module/reader.h>
#include <vistle/core/polygons.h>

#include <pnetcdf>
#include <vector>
#include <array>

constexpr int NUM_BLOCKS{2};
constexpr int NUM_SCALARS{4};

class ReadTsunami: public vistle::Reader {
public:
    //default constructor
    ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadTsunami() override;

private:
    typedef PnetCDF::NcmpiVar NcVar;
    typedef PnetCDF::NcmpiFile NcFile;
    typedef vistle::Vec<vistle::Scalar>::ptr VecScalarPtr;

    //structs
    template<class T>
    struct Dim {
        T dimLat;
        T dimLon;

        Dim(const T &lat, const T &lon): dimLat(lat), dimLon(lon) {}
    };

    template<class T>
    struct PolygonData {
        T numElements;
        T numCorners;
        T numVertices;

        PolygonData(const T &elem, const T &corn, const T &vert): numElements(elem), numCorners(corn), numVertices(vert)
        {}
    };

    struct NcVarExtended {
        MPI_Offset start;
        MPI_Offset count;
        MPI_Offset stride;
        MPI_Offset imap;

        NcVarExtended(const NcVar &nc, const MPI_Offset &start = 0, const MPI_Offset &count = 0,
                      const MPI_Offset &stride = 1, const MPI_Offset &imap = 1)
        : start(start), count(count), stride(stride), imap(imap)
        {
            ncVar = std::make_unique<NcVar>(nc);
        }

        template<class T>
        void readNcVar(T *storage) const
        {
            std::vector<MPI_Offset> v_start{start};
            std::vector<MPI_Offset> v_count{count};
            std::vector<MPI_Offset> v_stride{stride};
            std::vector<MPI_Offset> v_imap{imap};
            ncVar->getVar_all(v_start, v_count, v_stride, v_imap, storage);
        }

    private:
        std::shared_ptr<NcVar> ncVar;
    };

    //Vistle functions
    bool prepareRead() override;
    bool read(Token &token, int timestep, int block) override;
    bool finishRead() override;
    bool examine(const vistle::Parameter *param) override;

    //Own functions
    void initScalarParamReader();
    bool inspectNetCDFVars();

    typedef std::function<float(size_t, size_t)> ZCalcFunc;
    template<class U, class T, class V>
    void generateSurface(
        vistle::Polygons::ptr surface, const PolygonData<U> &polyData, const Dim<T> &dim, const std::vector<V> &coords,
        const ZCalcFunc &func = [](size_t x, size_t y) { return 0; });

    template<class T, class U>
    bool computeBlock(Token &token, const T &blockNum, const U &timestep);

    template<class Iter>
    void computeBlockPartition(const int blockNum, vistle::Index &nLatBlocks, vistle::Index &nLonBlocks,
                               Iter blockPartitionIterFirst);

    template<class T>
    bool computeInitial(Token &token, const T &blockNum);

    template<class T, class U>
    bool computeTimestep(Token &token, const T &blockNum, const U &timestep);
    void computeActualLastTimestep(const ptrdiff_t &incrementTimestep, const size_t &firstTimestep,
                                   size_t &lastTimestep, MPI_Offset &nTimesteps);

    template<class T, class PartionIdx>
    auto generateNcVarExt(const NcVar &ncVar, const T &dim, const T &ghost, const T &numDimBlocks,
                          const PartionIdx &partition) const;

    template<class T, class V>
    void contructLatLonSurface(vistle::Polygons::ptr poly, const Dim<T> &dim, const std::vector<V> &coords,
                               const ZCalcFunc &zCalc);

    template<class T>
    void fillConnectListPoly2Dim(vistle::Polygons::ptr poly, const Dim<T> &dim);

    template<class T>
    void fillPolyList(vistle::Polygons::ptr poly, const T &numCorner);

    void printMPIStats() const;
    void printThreadStats() const;
    template<class... Args>
    void printRank0(const std::string &str, Args... args) const;

    //Parameter
    vistle::IntParameter *m_ghost = nullptr;
    vistle::IntParameter *m_fill = nullptr;
    vistle::StringParameter *m_filedir = nullptr;
    vistle::StringParameter *m_bathy = nullptr;
    vistle::FloatParameter *m_verticalScale = nullptr;
    std::array<vistle::IntParameter *, NUM_BLOCKS> m_blocks{nullptr, nullptr};
    std::array<vistle::StringParameter *, NUM_SCALARS> m_scalars;

    //Ports
    vistle::Port *m_seaSurface_out = nullptr;
    vistle::Port *m_groundSurface_out = nullptr;
    std::array<vistle::Port *, NUM_SCALARS> m_scalarsOut;

    //helper variables
    int m_actualLastTimestep;
    std::atomic<bool> needSea = false;

    //Polygons per block
    std::map<int, vistle::Polygons::ptr> map_ptrSea;
    std::map<int, std::vector<float>> map_vecEta;

    //Scalar
    std::map<int, std::array<VecScalarPtr, NUM_SCALARS>> map_VecScalarPtr;

    //lat = 0; lon = 1
    std::array<std::string, NUM_BLOCKS> m_latLon_Sea;
    std::array<std::string, NUM_BLOCKS> m_latLon_Ground;
};
#endif
