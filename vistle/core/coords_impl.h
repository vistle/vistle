#ifndef COORDS_IMPL_H
#define COORDS_IMPL_H

namespace vistle {

template<class Archive>
void Coords::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_vec", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "normals", normals);
}

} // namespace vistle

#endif
