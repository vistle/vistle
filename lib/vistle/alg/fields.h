#ifndef VISTLE_ALG_FIELDS_H
#define VISTLE_ALG_FIELDS_H

#include "export.h"
#include <vistle/core/database.h>
#include <map>
#include <vector>

namespace vistle {

template<class Mapping>
DataBase::ptr remapData(typename DataBase::const_ptr &in, const Mapping &mapping, bool inverse = false);

extern template V_ALGEXPORT DataBase::ptr
remapData<std::vector<Index>>(typename DataBase::const_ptr &in, const std::vector<Index> &mapping, bool inverse);
extern template V_ALGEXPORT DataBase::ptr remapData<std::map<Index, Index>>(typename DataBase::const_ptr &in,
                                                                            const std::map<Index, Index> &mapping,
                                                                            bool inverse);
} // namespace vistle
#endif
