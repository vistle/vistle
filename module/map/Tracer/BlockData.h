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

public:
    const static int NumFields = 3;

    BlockData(vistle::Index i, vistle::Object::const_ptr grid, vistle::Vec<vistle::Scalar, 3>::const_ptr vdata,
              vistle::DataBase::const_ptr pdata[]);
    ~BlockData();

    const vistle::GridInterface *getGrid();
    vistle::Vec<vistle::Scalar, 3>::const_ptr getVecFld();
    vistle::DataBase::Mapping getVecMapping() const;

    const vistle::Matrix4 &transform() const;
    const vistle::Matrix4 &invTransform() const;
    const vistle::Matrix3 &velocityTransform() const;

private:
    vistle::Object::const_ptr m_grid;
    const vistle::GridInterface *m_gridInterface;
    vistle::Vec<vistle::Scalar, 3>::const_ptr m_vecfld;
    vistle::DataBase::const_ptr m_field[NumFields];
    vistle::DataBase::Mapping m_vecmap;
    const vistle::Scalar *m_vx, *m_vy, *m_vz;
    std::vector<vistle::DataBase::Mapping> m_scalmap;
    std::vector<const vistle::Scalar *> m_scal;
    vistle::Matrix4 m_transform, m_invTransform;
    vistle::Matrix3 m_velocityTransform;

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};
#endif
