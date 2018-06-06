#ifndef VISTLE_DATABASE_IMPL_H
#define VISTLE_DATABASE_IMPL_H

#include "scalars.h"

#include <limits>

#include <boost/mpl/size.hpp>

namespace vistle {

template<class Archive>
void DataBase::Data::serialize(Archive &ar) {
   ar & V_NAME(ar, "base_object", serialize_base<Base::Data>(ar, *this));
   ar & V_NAME(ar, "grid", grid);
   ar & V_NAME(ar, "mapping", mapping);
}

} // namespace vistle

#endif
