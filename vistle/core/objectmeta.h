#ifndef OBJECTMETA_H
#define OBJECTMETA_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>

#include <boost/serialization/access.hpp>

#include "shm.h"
#include "export.h"

//#define DEBUG_SERIALIZATION
#ifdef DEBUG_SERIALIZATION
template<class T>
inline const
boost::serialization::nvp<T> vistle_make_nvp(const char *name, T &t) {
   std::cerr << "<" << name << ">" << std::endl;
   return boost::serialization::make_nvp<T>(name, t);
}

#define V_NAME(name, obj) \
   vistle_make_nvp(name, (obj)); std::cerr << "</" << name << ">" << std::endl;
#else
#define V_NAME(name, obj) \
   boost::serialization::make_nvp(name, (obj))
#endif

namespace vistle {

class V_COREEXPORT Meta {
   public:
      Meta(int block=-1, int timestep=-1, int animationstep=-1, int iteration=-1, int execcount=-1, int creator=-1);
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
      int executionCounter() const { return m_executionCount; }
      void setExecutionCounter(int count) { m_executionCount = count; }
      int creator() const { return m_creator; }
      void setCreator(int id) { m_creator = id; }
   private:
      int m_block, m_numBlocks, m_timestep, m_numTimesteps, m_animationstep, m_numAnimationsteps, m_iteration, m_executionCount, m_creator;
      double m_realtime;

      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version);
};

V_COREEXPORT std::ostream &operator<<(std::ostream &out, const Meta &meta);

} // namespace vistle

#ifdef VISTLE_IMPL
#include "objectmeta_impl.h"
#endif
#endif
