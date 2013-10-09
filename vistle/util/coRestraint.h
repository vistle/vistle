#ifndef COVISE_RESTRAINT_H
#define COVISE_RESTRAINT_H
/**************************************************************************\ 
 **                                                                        **
 **                                                                        **
 ** Description: Interface classes for application modules to the COVISE   **
 **              software environment                                      **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                             (C)1997 RUS                                **
 **                Computing Center University of Stuttgart                **
 **                            Allmandring 30                              **
 **                            70550 Stuttgart                             **
 ** Author:                                                                **
 ** Date:                                                                  **
\**************************************************************************/

#include <vector>
#include <string>
#include <core/Index.h>

#include "export.h"
#include "sysdep.h"

namespace vistle
{
	
class V_UTILEXPORT coRestraint
{
   private:
      bool all;
      mutable std::vector<Index> values, min, max;
      mutable bool changed, stringCurrent;
      mutable std::string restraintString;

   public:
      coRestraint();
      ~coRestraint();

      void add(Index mi, Index ma);
      void add(Index val);
      void add(const std::string &selection);
      bool get(Index val, Index &group) const;
      size_t getNumGroups() const {return min.size();};
      void clear();
      const std::vector<Index> &getValues() const;
      Index lower() const;
      Index upper() const;
      const std::string &getRestraintString() const;
      const std::string getRestraintString(std::vector<Index>) const;

      // operators
      bool operator ()(Index val) const;
};

}
#endif                                            // COVISE_RESTRAINT_H
