#ifndef OBJECTMETA_IMPL_H
#define OBJECTMETA_IMPL_H

// include headers that implement an archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "serialize.h"

namespace vistle {

template<class Archive>
void Meta::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("block", m_block);
   ar & V_NAME("numblocks", m_numBlocks);
   ar & V_NAME("timestep", m_timestep);
   ar & V_NAME("numtimesteps", m_numTimesteps);
   ar & V_NAME("animationstep", m_animationstep);
   ar & V_NAME("iteration", m_iteration);
}

} // namespace vistle

#endif
