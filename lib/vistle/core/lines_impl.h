#ifndef LINES_IMPL_H
#define LINES_IMPL_H

namespace vistle {

template<class Archive>
void Lines::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_indexed", serialize_base<Base::Data>(ar, *this));
}

} // namespace vistle

#endif
