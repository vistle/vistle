#ifndef TUBES_IMPL_H
#define TUBES_IMPL_H

namespace vistle {

template<class Archive>
void Tubes::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_coords_with_radius", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "components", components);
    ar &V_NAME(ar, "style", style);
}

} // namespace vistle

#endif
