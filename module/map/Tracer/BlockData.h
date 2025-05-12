#ifndef VISTLE_TRACER_BLOCKDATA_H
#define VISTLE_TRACER_BLOCKDATA_H

#include <mutex>
#include <vector>

#include <vistle/core/grid.h>
#include <vistle/core/lines.h>
#include <vistle/core/object.h>

template<typename S>
class Particle;

class BlockData {
    friend class Particle<float>;
    friend class Particle<double>;

private:
    vistle::Object::const_ptr m_grid;
    const vistle::GridInterface *m_gridInterface;
    vistle::Vec<vistle::Scalar, 3>::const_ptr m_vecfld;
    vistle::Vec<vistle::Scalar>::const_ptr m_scafld;
    vistle::DataBase::Mapping m_vecmap, m_scamap;
    const vistle::Scalar *m_vx, *m_vy, *m_vz, *m_p;
    vistle::Matrix4 m_transform, m_invTransform;
    vistle::Matrix3 m_velocityTransform;

public:
    BlockData(vistle::Index i, vistle::Object::const_ptr grid, vistle::Vec<vistle::Scalar, 3>::const_ptr vdata,
              vistle::Vec<vistle::Scalar>::const_ptr pdata = nullptr);
    ~BlockData();

    const vistle::GridInterface *getGrid();
    vistle::Vec<vistle::Scalar, 3>::const_ptr getVecFld();
    vistle::DataBase::Mapping getVecMapping() const;
    vistle::Vec<vistle::Scalar>::const_ptr getScalFld();
    vistle::DataBase::Mapping getScalMapping() const;
    std::vector<vistle::Vec<vistle::Scalar, 3>::ptr> getIplVec();
    std::vector<vistle::Vec<vistle::Scalar>::ptr> getIplScal();

    const vistle::Matrix4 &transform() const;
    const vistle::Matrix4 &invTransform() const;
    const vistle::Matrix3 &velocityTransform() const;

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};
#endif
