#ifndef VISTLE_READER_H
#define VISTLE_READER_H

#include "module.h"
#include <set>

namespace vistle {

class V_MODULEEXPORT Reader: public Module {

public:
   Reader(const std::string &description,
          const std::string &name, const int moduleID, mpi::communicator comm);
   ~Reader() override;
   bool prepare() override;
   bool compute() override;

   virtual bool examine(const Parameter *param);
   virtual bool read(const Meta &meta, int timestep=-1, int block=-1) = 0;
   virtual bool prepareRead();
   virtual bool finishRead();

   int timeIncrement() const;

protected:
   void setHandlePartitions(bool enable);
   void setAllowTimestepDistribution(bool allow);

   void observeParameter(const Parameter *param);
   void setTimesteps(int number);
   void setPartitions(int number);

   bool changeParameter(const Parameter *param) override;
   void prepareQuit() override;

   bool checkConvexity() const;

   virtual int rankForTimestepAndPartition(int t, int p) const;

   IntParameter *m_first = nullptr;
   IntParameter *m_last = nullptr;
   IntParameter *m_increment = nullptr;
   IntParameter *m_distributeTime = nullptr;
   IntParameter *m_firstRank = nullptr;
   IntParameter *m_checkConvexity = nullptr;

private:
   std::set<const Parameter *> m_observedParameters;

   int m_numTimesteps = 0;
   int m_numPartitions = 0;
   bool m_readyForRead = true;

   bool m_handlePartitions = true;
   bool m_allowTimestepDistribution = false;

   /*
    * # files (api)
    * file selection (ui)
    * # data ports/fields (api)
    * field -> port mapping (ui)
    * bool: directory/file (api)
    * timestep control (ui)
    * distribution over ranks (ui)
    * ? bool: can reorder (api)
    * ? extensions? (api)
    * ? bool: field name from file (api)
    * ? bool: create ghostcells (ui)
    */
};

}
#endif
