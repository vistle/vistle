#ifndef VISTLE_ISOSURFACEVTKM_ISOSURFACEVTKM_H
#define VISTLE_ISOSURFACEVTKM_ISOSURFACEVTKM_H

#include <vistle/module/module.h>

class IsoSurfaceVtkm: public vistle::Module {
public:
    IsoSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~IsoSurfaceVtkm();

private:
    struct BlockData {
        BlockData(vistle::Object::const_ptr g, vistle::DataBase::const_ptr d, vistle::DataBase::const_ptr m)
        : grid(g), datas(d), mapdata(m)
        {}
        int getTimestep() const;
        vistle::Object::const_ptr grid;
        vistle::DataBase::const_ptr datas;
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

    vistle::Port *m_mapDataIn = nullptr, *m_dataOut = nullptr;
    vistle::FloatParameter *m_isovalue = nullptr;
    vistle::VectorParameter *m_isopoint = nullptr;
    vistle::IntParameter *m_computeNormals = nullptr;
    vistle::IntParameter *m_pointOrValue = nullptr;

    bool changeParameter(const vistle::Parameter *param) override;
    void setInputSpecies(const std::string &species) override;
    void updateModuleInfo();
    bool prepare() override;
    bool reduce(int timestep) override;
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    vistle::Object::ptr work(vistle::Object::const_ptr grid, vistle::DataBase::const_ptr isodata,
                             vistle::DataBase::const_ptr mapdata, vistle::Scalar isoValue = 0.) const;

    mutable vistle::Scalar m_min, m_max;
    vistle::Float m_paraMin, m_paraMax;
    bool m_performedPointSearch = false;
    bool m_foundPoint = false;
    std::string m_species;
};

#endif
