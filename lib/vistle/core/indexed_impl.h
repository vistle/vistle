#ifndef INDEXED_IMPL_H
#define INDEXED_IMPL_H

namespace vistle {

template<class Archive>
void Indexed::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_coords", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "element_list", el);
    ar &V_NAME(ar, "connection_list", cl);
    ar &V_NAME(ar, "ghost_list", ghost);
}

} // namespace vistle

#endif
