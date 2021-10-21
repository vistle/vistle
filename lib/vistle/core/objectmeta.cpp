#include "shm_array.h"
#include "objectmeta.h"
#include "objectmeta_impl.h"
#include "archives.h"

namespace vistle {

Meta::Meta(int block, int timestep, int animstep, int iteration, int execcount, int creator)
: m_block(block)
, m_numBlocks(-1)
, m_timestep(timestep)
, m_numTimesteps(-1)
, m_animationstep(animstep)
, m_numAnimationsteps(-1)
, m_iteration(iteration)
, m_executionCount(execcount)
, m_creator(creator)
, m_realtime(0.0)
{
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m_transform[i * 4 + j] = i == j ? 1. : 0.;
}

Matrix4 Meta::transform() const
{
    Matrix4 t;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            t.coeffRef(i, j) = m_transform[i * 4 + j];
        }
    }
    return t;
}

void Meta::setTransform(const Matrix4 &transform)
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m_transform[i * 4 + j] = transform.coeff(i, j);
        }
    }
}

std::ostream &operator<<(std::ostream &out, const Meta &meta)
{
    out << "creator: " << meta.creator() << ", exec: " << meta.executionCounter() << ", block: " << meta.block() << "/"
        << meta.numBlocks() << ", time: " << meta.timeStep() << "/" << meta.numTimesteps();
    return out;
}

#ifdef USE_BOOST_ARCHIVE
template void V_COREEXPORT Meta::serialize<boost_iarchive>(boost_iarchive &ar);
template void V_COREEXPORT Meta::serialize<boost_oarchive>(boost_oarchive &ar);
#endif
#ifdef USE_YAS
template void V_COREEXPORT Meta::serialize<yas_iarchive>(yas_iarchive &ar);
template void V_COREEXPORT Meta::serialize<yas_oarchive>(yas_oarchive &ar);
#endif

} // namespace vistle
