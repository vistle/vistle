#ifndef VISTLE_CORE_POINTS_IMPL_H
#define VISTLE_CORE_POINTS_IMPL_H

namespace vistle {

template<class Archive>
void Points::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_coords", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "radius", radius);
}

} // namespace vistle

#endif
