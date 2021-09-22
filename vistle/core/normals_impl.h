#ifndef NORMALS_IMPL_H
#define NORMALS_IMPL_H

namespace vistle {

template<class Archive>
void Normals::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_vec", serialize_base<Base::Data>(ar, *this));
}

} // namespace vistle

#endif
