#ifndef EMPTY_IMPL_H
#define EMPTY_IMPL_H

#include "archives_config.h"

namespace vistle {

template<class Archive>
void Empty::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_object", serialize_base<Base::Data>(ar, *this));
}

} // namespace vistle

#endif
