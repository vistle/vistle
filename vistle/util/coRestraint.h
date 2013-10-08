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
#include <core/index.h>

#include "export.h"

namespace vistle
{
	
class V_UTILEXPORT coRestraint
{
   private:
      bool all;
      mutable std::vector<index> values, min, max;
      mutable bool changed, stringCurrent;
      mutable std::string restraintString;

   public:
      coRestraint();
      ~coRestraint();

      void add(index mi, index ma);
      void add(index val);
      void add(const std::string &selection);
      bool get(index val, index &group) const;
      size_t getNumGroups() const {return min.size();};
      void clear();
      const std::vector<index> &getValues() const;
      index lower() const;
      index upper() const;
      const std::string &getRestraintString() const;
      const std::string getRestraintString(std::vector<index>) const;

      // operators
      bool operator ()(index val) const;
};

}
#endif                                            // COVISE_RESTRAINT_H
