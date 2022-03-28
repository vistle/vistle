#ifndef ISOSURFACE_H
#define ISOSURFACE_H

#include <vistle/module/module.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include "IsoDataFunctor.h"

#ifdef CUTTINGSURFACE
#include <vistle/module/resultcache.h>
#endif

class IsoSurface: public vistle::Module {
public:
    IsoSurface(const std::string &name, int moduleID, mpi::communicator comm);
    ~IsoSurface();

private:
    std::pair<vistle::Object::ptr, vistle::Object::ptr>
    generateIsoSurface(vistle::Object::const_ptr grid, vistle::Object::const_ptr data,
                       vistle::Object::const_ptr mapdata, const vistle::Scalar isoValue, int processorType);

    struct BlockData {
        BlockData(vistle::Object::const_ptr g, vistle::Vec<vistle::Scalar>::const_ptr d, vistle::DataBase::const_ptr m)
        : grid(g), datas(d), mapdata(m)
        {}
        int getTimestep() const;
        vistle::Object::const_ptr grid;
        vistle::Vec<vistle::Scalar>::const_ptr datas;
        vistle::DataBase::const_ptr mapdata;

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

    vistle::Object::ptr work(vistle::Object::const_ptr grid, vistle::Vec<vistle::Scalar>::const_ptr data,
                             vistle::DataBase::const_ptr mapdata, vistle::Scalar isoValue = 0.) const;
    bool compute(std::shared_ptr<vistle::BlockTask> task) const override;
    //bool compute() override;
    bool prepare() override;
    bool reduce(int timestep) override;
    bool changeParameter(const vistle::Parameter *param) override;

    vistle::FloatParameter *m_isovalue;
    vistle::VectorParameter *m_isopoint;
    vistle::IntParameter *m_pointOrValue;
    vistle::IntParameter *m_processortype;
    vistle::IntParameter *m_computeNormals;

    vistle::StringParameter *m_heightmap;
    vistle::Port *m_mapDataIn, *m_dataOut;

    mutable vistle::Scalar m_min, m_max;
    vistle::Float m_paraMin, m_paraMax;
    bool m_performedPointSearch = false;
    bool m_foundPoint = false;

    IsoController isocontrol;

#ifdef CUTTINGSURFACE
    mutable vistle::ResultCache<vistle::Coords::ptr> m_gridCache;
#endif
};

#endif
