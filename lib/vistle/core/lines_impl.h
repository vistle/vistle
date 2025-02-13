#ifndef VISTLE_CORE_LINES_IMPL_H
#define VISTLE_CORE_LINES_IMPL_H

namespace vistle {

template<class Archive>
void Lines::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_indexed", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "radius", radius);
    ar &V_NAME(ar, "style", style);
}

} // namespace vistle

#endif
