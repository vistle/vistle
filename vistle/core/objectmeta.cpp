#include "objectmeta.h"
#include "objectmeta_impl.h"

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
}

} // namespace vistle
