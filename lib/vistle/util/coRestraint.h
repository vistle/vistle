#ifndef VISTLE_UTIL_CORESTRAINT_H
#define VISTLE_UTIL_CORESTRAINT_H

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
#include <cstdlib>

#include "export.h"

#include "ssize_t.h"

namespace vistle {

class V_UTILEXPORT coRestraint {
private:
    bool all;
    int globalStep = 1;
    mutable std::vector<ssize_t> values, min, max, step;
    mutable bool changed, stringCurrent;
    mutable std::string restraintString;

public:
    coRestraint();
    ~coRestraint();

    void add(ssize_t mi, ssize_t ma, ssize_t step = 1);
    void add(ssize_t val);
    void add(const std::string &selection);
    void cut();
    bool get(ssize_t val, ssize_t &group) const;
    size_t getNumGroups() const;
    void clear();
    const std::vector<ssize_t> &getValues() const;
    ssize_t lower() const;
    ssize_t upper() const;
    const std::string &getRestraintString() const;
    const std::string getRestraintString(std::vector<ssize_t>) const;

    // operators
    bool operator()(ssize_t val) const;
};

} // namespace vistle
#endif
