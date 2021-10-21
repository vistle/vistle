#ifndef SPHERES_IMPL_H
#define SPHERES_IMPL_H

namespace vistle {

template<class Archive>
void Spheres::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_coords_with_radius", serialize_base<Base::Data>(ar, *this));
}

} // namespace vistle

#endif
