#ifndef PLACEHOLDER_IMPL_H
#define PLACEHOLDER_IMPL_H

#include "archives_config.h"

namespace vistle {

template<class Archive>
void PlaceHolder::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_object", serialize_base<Base::Data>(ar, *this));

    ar &V_NAME(ar, "orig_name", originalName);
    ar &V_NAME(ar, "orig_meta", originalMeta);
    ar &V_NAME(ar, "orig_type", originalType);

    ar &V_NAME(ar, "geometry", geometry);
    ar &V_NAME(ar, "normals", normals);
    ar &V_NAME(ar, "texture", texture);
}

} // namespace vistle

#endif
