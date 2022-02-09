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
    ReadTsunami(const string &name, int moduleID, mpi::communicator comm);
    ~ReadTsunami() override;

private:
    typedef PnetCDF::NcmpiVar NcVar;
    typedef PnetCDF::NcmpiFile NcFile;
    typedef unique_ptr<NcFile> NcFilePtr;
    typedef vistle::Vec<vistle::Scalar>::ptr VisVecScalarPtr;
    typedef array<VisVecScalarPtr, NUM_SCALARS> ArrVecScalarPtrs;
    typedef vector<ArrVecScalarPtrs> VecArrVecScalarPtrs;
    typedef vector<array<float, 2>> VecLatLon;
    typedef function<float(size_t, size_t)> ZCalcFunc;

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
            ncVar = make_unique<NcVar>(nc);
        }

        auto &Start() const { return start; }
        auto &Count() const { return count; }
        auto &Stride() const { return stride; }
        auto &Imap() const { return imap; }

        template<class T>
        void readNcVar(T *storage) const
        {
            const vector<NcParamType> v_start{start}, v_count{count}, v_stride{stride}, v_imap{imap};
            read(storage, v_start, v_count, v_stride, v_imap);
        }

    private:
        template<class T, class... Args>
        void read(T *storage, Args... args) const
        {
            ncVar->getVar_all(args..., storage);
        }
        unique_ptr<NcVar> ncVar;
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
    void initETA(const NcFilePtr &ncFile, const array<PNcVarExt, 2> &ncExtSea, const ReaderTime &time,
                 const size_t &verticesSea, int block);
    void initSea(const NcFilePtr &ncFile, const array<vistle::Index, 2> &nBlocks,
                 const array<vistle::Index, NUM_BLOCKS> &blockPartIdx, const ReaderTime &time, int ghost, int block);
    void initScalars(const NcFilePtr &ncFile, const array<PNcVarExt, 2> &ncExtSea, const size_t &verticesSea,
                     int block);
    void createGround(Token &token, const NcFilePtr &ncFile, const array<vistle::Index, 2> &nBlocks,
                      const array<vistle::Index, NUM_BLOCKS> &blockPartIdx, int ghost, int block);
    void initScalarParamReader();
    bool inspectNetCDF();
    bool inspectDims(const NcFilePtr &ncFile);
    bool inspectScalars(const NcFilePtr &ncFile);
    NcFilePtr openNcmpiFile();

    bool computeConst(Token &token, const int block);
    bool computeTimestep(Token &token, const int block, const int timestep);
    bool computeBlock(Token &token, const int block, const int timestep);

    template<class Iter>
    void computeBlockPartition(const int block, vistle::Index &nLatBlocks, vistle::Index &nLonBlocks,
                               Iter blockPartitionIterFirst);
    template<class T>
    void fillHeight(
        vistle::LayerGrid::ptr surface, const Dim<T> &dim,
        const ZCalcFunc &func = [](size_t x, size_t y) { return 0; });

    template<class T, class PartionIdx>
    auto generateNcVarExt(const NcVar &ncVar, const T &dim, const T &ghost, const T &numDimBlocks,
                          const PartionIdx &partition) const;

    template<class... Args>
    void printRank0(const string &str, Args... args) const;
    void printMPIStats() const;
    void printThreadStats() const;

    //Parameter
    vistle::IntParameter *m_ghost = nullptr;
    vistle::IntParameter *m_fill = nullptr;
    vistle::StringParameter *m_filedir = nullptr;
    vistle::StringParameter *m_bathy = nullptr;
    vistle::FloatParameter *m_verticalScale = nullptr;
    array<vistle::IntParameter *, NUM_BLOCKS> m_blocks{nullptr, nullptr};
    array<vistle::StringParameter *, NUM_SCALARS> m_scalars;

    //Ports
    vistle::Port *m_seaSurface_out = nullptr;
    vistle::Port *m_groundSurface_out = nullptr;
    array<vistle::Port *, NUM_SCALARS> m_scalarsOut;

    //*****helper variables*****//
    atomic_bool m_needSea;
    mutex m_mtx;
    boost::mpi::intercommunicator m_pnetcdf_comm;

    //per block
    vector<int> m_block_etaIdx;
    vector<moffDim> m_block_dimSea;
    vector<vector<float>> m_block_etaVec;
    VecArrVecScalarPtrs m_block_VecScalarPtr;
    VecLatLon m_block_min;
    VecLatLon m_block_max;

    //lat = 0; lon = 1
    array<string, NUM_BLOCKS> m_latLon_Sea;
    array<string, NUM_BLOCKS> m_latLon_Ground;
};
#endif
