#ifndef OBJECTMETA_IMPL_H
#define OBJECTMETA_IMPL_H

#include <cassert>
#include "serialize.h"
#include "archives_config.h"

namespace vistle {

template<class Archive>
void Meta::serialize(Archive &ar)
{
#ifdef DEBUG_SERIALIZATION
    const unsigned int check = 0xdeadbeef;
    unsigned int check1 = check;
    ar &V_NAME(ar, "check1", check1);
    assert(check1 == check);
#endif
    ar &V_NAME(ar, "block", m_block);
    ar &V_NAME(ar, "numblocks", m_numBlocks);
    ar &V_NAME(ar, "timestep", m_timestep);
    ar &V_NAME(ar, "numtimesteps", m_numTimesteps);
    ar &V_NAME(ar, "animationstep", m_animationstep);
    ar &V_NAME(ar, "numanimationsteps", m_numAnimationsteps);
    ar &V_NAME(ar, "iteration", m_iteration);
    ar &V_NAME(ar, "creator", m_creator);
    ar &V_NAME(ar, "executioncount", m_executionCount);
    ar &V_NAME(ar, "realtime", m_realtime);
    for (unsigned i = 0; i < m_transform.size(); ++i)
        ar &V_NAME(ar, "transform", m_transform[i]);
#ifdef DEBUG_SERIALIZATION
    unsigned int check2 = check;
    ar &V_NAME(ar, "check2", check2);
    assert(check2 == check);
#endif
}

} // namespace vistle

#endif
