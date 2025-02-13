#ifndef VISTLE_CORE_TEXTURE1D_IMPL_H
#define VISTLE_CORE_TEXTURE1D_IMPL_H

namespace vistle {

template<class Archive>
void Texture1D::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_vec", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "pixels", pixels);
    ar &V_NAME(ar, "min", range[0]);
    ar &V_NAME(ar, "max", range[1]);
}

} // namespace vistle
#endif
