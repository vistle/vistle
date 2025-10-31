#ifndef VISTLE_ISOSURFACEVTKM_ISOSURFACEVTKM_H
#define VISTLE_ISOSURFACEVTKM_ISOSURFACEVTKM_H

#include <vistle/module/module.h>

class IsoSurfaceVtkm: public vistle::Module {
public:
    IsoSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    static const int NumPorts = 3;

private:
    struct BlockData {
        BlockData(vistle::Object::const_ptr g, const std::vector<vistle::DataBase::const_ptr> &m): grid(g), mapdata(m)
        {}
        int getTimestep() const;
        vistle::Object::const_ptr grid;
        std::vector<vistle::DataBase::const_ptr> mapdata;

        bool operator<(const BlockData &rhs)
        {
            if (mapdata[0] == rhs.mapdata[0]) {
                if (mapdata[1] == rhs.mapdata[1]) {
                    return grid < rhs.grid;
                }
                return mapdata[1] < rhs.mapdata[1];
            }
            return mapdata[0] < rhs.mapdata[0];
        }
    };

    mutable std::mutex m_mutex;
    mutable std::map<int, std::vector<BlockData>> m_blocksForTime;

    vistle::Port *m_dataIn[NumPorts], *m_dataOut[NumPorts];
    vistle::Port *m_surfOut = nullptr;
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

    std::vector<vistle::Object::ptr> work(vistle::Object::const_ptr grid,
                                          const std::vector<vistle::DataBase::const_ptr> &mapdata,
                                          vistle::Scalar isoValue) const;

    mutable vistle::Scalar m_min, m_max;
    vistle::Float m_paraMin, m_paraMax;
    bool m_performedPointSearch = false;
    bool m_foundPoint = false;
    std::string m_species;
};

#endif
