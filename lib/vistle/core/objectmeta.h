#ifndef VISTLE_CORE_OBJECTMETA_H
#define VISTLE_CORE_OBJECTMETA_H

#include <array>
#include <vector>
#include <memory>

#include "archives_config.h"

#include "export.h"
#include "shm.h"
#include "vectortypes.h"


namespace vistle {

class V_COREEXPORT Meta {
public:
    Meta(int block = -1, int timestep = -1, int animationstep = -1, int iteration = -1, int generation = -1,
         int creator = -1);
    int block() const { return m_block; }
    void setBlock(int block) { m_block = block; }
    int numBlocks() const { return m_numBlocks; }
    void setNumBlocks(int num) { m_numBlocks = num; }
    double realTime() const { return m_realtime; }
    void setRealTime(double time) { m_realtime = time; }
    int timeStep() const { return m_timestep; }
    void setTimeStep(int timestep) { m_timestep = timestep; }
    int numTimesteps() const { return m_numTimesteps; }
    void setNumTimesteps(int num) { m_numTimesteps = num; }
    int animationStep() const { return m_animationstep; }
    void setAnimationStep(int step) { m_animationstep = step; }
    void setNumAnimationSteps(int step) { m_numAnimationsteps = step; }
    int numAnimationSteps() const { return m_numAnimationsteps; }
    int iteration() const { return m_iteration; }
    void setIteration(int iteration) { m_iteration = iteration; }
    int generation() const { return m_generation; }
    void setGeneration(int count) { m_generation = count; }
    int creator() const { return m_creator; }
    void setCreator(int id) { m_creator = id; }
    Matrix4 transform() const;
    void setTransform(const Matrix4 &transform);

private:
    int32_t m_block, m_numBlocks, m_timestep, m_numTimesteps, m_animationstep, m_numAnimationsteps, m_iteration,
        m_generation, m_creator;
    double m_realtime;
    std::array<double, 16> m_transform;

    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar);
};

V_COREEXPORT std::ostream &operator<<(std::ostream &out, const Meta &meta);

#ifdef USE_BOOST_ARCHIVE
extern template void V_COREEXPORT Meta::serialize<boost_iarchive>(boost_iarchive &ar);
extern template void V_COREEXPORT Meta::serialize<boost_oarchive>(boost_oarchive &ar);
#endif
#ifdef USE_YAS
extern template void V_COREEXPORT Meta::serialize<yas_iarchive>(yas_iarchive &ar);
extern template void V_COREEXPORT Meta::serialize<yas_oarchive>(yas_oarchive &ar);
#endif

} // namespace vistle
#endif
