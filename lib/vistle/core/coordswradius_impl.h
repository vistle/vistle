#ifndef COORDSWITHRADIUS_IMPL_H
#define COORDSWITHRADIUS_IMPL_H

namespace vistle {

template<class Archive>
void CoordsWithRadius::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_coords", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "radii", r);
}

} // namespace vistle

#endif
