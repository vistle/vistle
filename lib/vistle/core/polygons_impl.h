#ifndef VISTLE_CORE_POLYGONS_IMPL_H
#define VISTLE_CORE_POLYGONS_IMPL_H

namespace vistle {

template<class Archive>
void Polygons::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_indexed", serialize_base<Base::Data>(ar, *this));
}

} // namespace vistle

#endif
