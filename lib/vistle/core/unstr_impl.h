#ifndef UNSTR_IMPL_H
#define UNSTR_IMPL_H

namespace vistle {

template<class Archive>
void UnstructuredGrid::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_indexed", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "type_list", tl);
}

} // namespace vistle

#endif
