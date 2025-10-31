#ifndef VISTLE_ISOSURFACE_ISOSURFACE_H
#define VISTLE_ISOSURFACE_ISOSURFACE_H

#include <vistle/module/module.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include "IsoDataFunctor.h"

#ifdef CUTTINGSURFACE
#include <vistle/module/resultcache.h>
#endif

class IsoSurface: public vistle::Module {
#if defined(CUTTINGSURFACE)
    constexpr static int NumPorts = 3;
#elif defined(ISOHEIGHTSURFACE)
    constexpr static int NumPorts = 1;
#else
    constexpr static int NumPorts = 3;
#endif

public:
    IsoSurface(const std::string &name, int moduleID, mpi::communicator comm);

private:
    std::pair<vistle::Object::ptr, vistle::Object::ptr>
    generateIsoSurface(vistle::Object::const_ptr grid, vistle::Object::const_ptr data,
                       vistle::Object::const_ptr mapdata, const vistle::Scalar isoValue, int processorType);

    struct BlockData {
        BlockData(vistle::Object::const_ptr g, vistle::Vec<vistle::Scalar>::const_ptr d,
                  const std::vector<vistle::DataBase::const_ptr> &m)
        : grid(g), datas(d), mapdata(m)
        {}
        int getTimestep() const;
        vistle::Object::const_ptr grid;
        vistle::Vec<vistle::Scalar>::const_ptr datas;
        std::vector<vistle::DataBase::const_ptr> mapdata;

        bool operator<(const BlockData &rhs)
        {
            if (datas == rhs.datas) {
                if (mapdata == rhs.mapdata) {
                    return grid < rhs.grid;
                }
                return mapdata < rhs.mapdata;
            }
            return datas < rhs.datas;
        }
    };

    mutable std::mutex m_mutex;
    mutable std::map<int, std::vector<BlockData>> m_blocksForTime;

    vistle::Object::ptr createHeightCut(vistle::Object::const_ptr grid, vistle::Vec<vistle::Scalar>::const_ptr data,
                                        vistle::DataBase::const_ptr mapdata) const;

    std::vector<vistle::Object::ptr> work(vistle::Object::const_ptr grid, vistle::Vec<vistle::Scalar>::const_ptr data,
                                          std::vector<vistle::DataBase::const_ptr> mapdata,
                                          vistle::Scalar isoValue = 0.) const;
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
    //bool compute() override;
    bool prepare() override;
    bool reduce(int timestep) override;
    bool changeParameter(const vistle::Parameter *param) override;
    void setInputSpecies(const std::string &species) override;

    void updateModuleInfo();

    vistle::FloatParameter *m_isovalue;
    vistle::VectorParameter *m_isopoint;
    vistle::IntParameter *m_pointOrValue;
    vistle::IntParameter *m_computeNormals;

    vistle::StringParameter *m_heightmap;
    vistle::Port *m_dataIn[NumPorts], *m_dataOut[NumPorts];
    vistle::Port *&m_mapDataIn = m_dataIn[0];
#ifdef ISOSURFACE
    vistle::Port *m_surfOut = nullptr;
#endif

    mutable vistle::Scalar m_min, m_max;
    vistle::Float m_paraMin, m_paraMax;
    bool m_performedPointSearch = false;
    bool m_foundPoint = false;
    std::string m_species;

    IsoController isocontrol;

#ifdef CUTTINGSURFACE
    struct CachedResult {
        std::shared_ptr<const std::vector<vistle::Index>> candidateCells;
        vistle::Coords::ptr grid;
    };
    mutable vistle::ResultCache<CachedResult> m_gridCache;
#endif
};

#endif
