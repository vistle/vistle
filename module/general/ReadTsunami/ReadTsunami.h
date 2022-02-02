/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module for ChEESE tsunami nc-files         	       **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:    Marko Djuric <hpcmdjur@hlrs.de>                             **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Date:  25.01.2021 Version 1 with netCDF                                **
 ** Date:  29.10.2021 Version 2 with PnetCDF                               **
 ** Date:  24.01.2022 Version 2.1 use layergrid instead of polygons        **
 ** Date:  25.01.2022 Version 2.2 optimize creation of surfaces            **
\**************************************************************************/

#ifndef _READTSUNAMI_H
#define _READTSUNAMI_H

#include "vistle/core/index.h"
#include <atomic>
#include <memory>
#include <mpi.h>
#include <mutex>
#include <vistle/module/reader.h>
#include <vistle/core/layergrid.h>

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
    typedef vistle::Vec<vistle::Scalar>::ptr VisVecScalarPtr;
    typedef std::array<VisVecScalarPtr, NUM_SCALARS> ArrVecScalarPtrs;
    typedef std::vector<ArrVecScalarPtrs> VecArrVecScalarPtrs;
    typedef std::vector<std::array<float, 2>> VecLatLon;
    typedef std::function<float(size_t, size_t)> ZCalcFunc;

    //structs
    template<class T>
    struct Dim {
        Dim(const T &l_x = 0, const T &l_y = 0, const T &l_z = 0): x(l_x), y(l_y), z(l_z) {}
        auto &X() const { return x; }
        auto &Y() const { return y; }
        auto &Z() const { return z; }

    private:
        T x;
        T y;
        T z;
    };
    typedef Dim<MPI_Offset> moffDim;
    typedef Dim<size_t> sztDim;

    template<class NcParamType>
    struct NcVarExt {
        NcVarExt() = default;
        NcVarExt(const NcVar &nc, const NcParamType &start = 0, const NcParamType &count = 0,
                 const NcParamType &stride = 1, const NcParamType &imap = 1)
        : start(start), count(count), stride(stride), imap(imap)
        {
            ncVar = std::make_unique<NcVar>(nc);
        }

        auto &Start() const { return start; }
        auto &Count() const { return count; }
        auto &Stride() const { return stride; }
        auto &Imap() const { return imap; }

        template<class T>
        void readNcVar(T *storage) const
        {
            const std::vector<NcParamType> v_start{start}, v_count{count}, v_stride{stride}, v_imap{imap};
            read(storage, v_start, v_count, v_stride, v_imap);
        }

    private:
        template<class T, class... Args>
        void read(T *storage, Args... args) const
        {
            ncVar->getVar_all(args..., storage);
        }
        std::unique_ptr<NcVar> ncVar;
        NcParamType start;
        NcParamType count;
        NcParamType stride;
        NcParamType imap;
    };
    typedef NcVarExt<MPI_Offset> PNcVarExt;

    //Vistle functions
    bool prepareRead() override;
    bool read(Token &token, int timestep, int block) override;
    bool finishRead() override;
    bool examine(const vistle::Parameter *param) override;

    //Own functions
    void initETA(const NcFile *ncFile, const std::array<PNcVarExt, 2> &ncExtSea, const ReaderTime &time,
                 const size_t &verticesSea, int block);
    void initSea(const NcFile *ncFile, const std::array<vistle::Index, 2> &nBlocks,
                 const std::array<vistle::Index, NUM_BLOCKS> &blockPartIdx, const ReaderTime &time, int ghost,
                 int block);
    void initScalars(const NcFile *ncFile, const std::array<PNcVarExt, 2> &ncExtSea, const size_t &verticesSea,
                     int block);
    void createGround(Token &token, const NcFile *ncFile, const std::array<vistle::Index, 2> &nBlocks,
                      const std::array<vistle::Index, NUM_BLOCKS> &blockPartIdx, int ghost, int block);
    void initScalarParamReader();
    bool inspectNetCDF();
    bool inspectDims(const NcFile *ncFile);
    bool inspectScalars(const NcFile *ncFile);
    std::unique_ptr<NcFile> openNcmpiFile();

    template<class T>
    void fillHeight(
        vistle::LayerGrid::ptr surface, const Dim<T> &dim,
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

    template<class T, class PartionIDs>
    auto generateNcVarExt(const NcVar &ncVar, const T &dim, const T &ghost, const T &numDimBlocks,
                          const PartionIDs &partitionIDs) const;

    template<class... Args>
    void printRank0(const std::string &str, Args... args) const;
    void printMPIStats() const;
    void printThreadStats() const;

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

    //*****helper variables*****//
    std::atomic_bool m_needSea;
    std::mutex m_mtx;
    boost::mpi::intercommunicator m_pnetcdf_comm;

    //per block
    std::vector<int> m_block_etaIdx;
    std::vector<moffDim> m_block_dimSea;
    std::vector<std::vector<float>> m_block_etaVec;
    VecArrVecScalarPtrs m_block_VecScalarPtr;
    VecLatLon m_block_min;
    VecLatLon m_block_max;

    //lat = 0; lon = 1
    std::array<std::string, NUM_BLOCKS> m_latLon_Sea;
    std::array<std::string, NUM_BLOCKS> m_latLon_Ground;
};
#endif
