#ifndef TRACER_BLOCKDATA_H
#define TRACER_BLOCKDATA_H

#include <vector>

#include <core/object.h>
#include <core/grid.h>
#include <core/lines.h>
#include <mutex>


class BlockData{

    friend class Particle;

private:
    vistle::Object::const_ptr m_grid;
    const vistle::GridInterface *m_gridInterface;
    vistle::Vec<vistle::Scalar, 3>::const_ptr m_vecfld;
    vistle::Vec<vistle::Scalar>::const_ptr m_scafld;
    vistle::DataBase::Mapping m_vecmap, m_scamap;
    vistle::Lines::ptr m_lines;
    std::vector<vistle::Vec<vistle::Scalar, 3>::ptr> m_ivec;
    std::vector<vistle::Vec<vistle::Scalar>::ptr> m_iscal;
    vistle::Vec<vistle::Index>::ptr m_ids;
    vistle::Vec<vistle::Index>::ptr m_steps;
    vistle::Vec<vistle::Scalar>::ptr m_times, m_dists;
    const vistle::Scalar *m_vx, *m_vy, *m_vz, *m_p;
    std::mutex m_mutex;

public:
    BlockData(vistle::Index i,
              vistle::Object::const_ptr grid,
              vistle::Vec<vistle::Scalar, 3>::const_ptr vdata,
              vistle::Vec<vistle::Scalar>::const_ptr pdata = nullptr);
    ~BlockData();

    void setMeta(const vistle::Meta &meta);
    const vistle::GridInterface *getGrid();
    vistle::Vec<vistle::Index>::ptr ids() const;
    vistle::Vec<vistle::Index>::ptr steps() const;
    vistle::Vec<vistle::Scalar>::ptr times() const;
    vistle::Vec<vistle::Scalar>::ptr distances() const;
    vistle::Vec<vistle::Scalar, 3>::const_ptr getVecFld();
    vistle::DataBase::Mapping getVecMapping() const;
    vistle::Vec<vistle::Scalar>::const_ptr getScalFld();
    vistle::DataBase::Mapping getScalMapping() const;
    vistle::Lines::ptr getLines();
    std::vector<vistle::Vec<vistle::Scalar, 3>::ptr> getIplVec();
    std::vector<vistle::Vec<vistle::Scalar>::ptr> getIplScal();
    void addLines(vistle::Index id, const std::vector<vistle::Vector3> &points,
                 const std::vector<vistle::Vector3> &velocities,
                 const std::vector<vistle::Scalar> &pressures,
                 const std::vector<vistle::Index> &steps,
                 const std::vector<vistle::Scalar> &times,
                 const std::vector<vistle::Scalar> &dists);
};
#endif
