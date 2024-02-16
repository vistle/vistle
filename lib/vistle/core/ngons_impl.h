#ifndef VISTLE_NGONS_IMPL_H
#define VISTLE_NGONS_IMPL_H

namespace vistle {

template<int N>
template<class Archive>
void Ngons<N>::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_coords", serialize_base<Base::Data>(ar, *this));
    ar &V_NAME(ar, "connection_list", cl);
    ar &V_NAME(ar, "ghost_list", ghost);
}

template<int N>
void Ngons<N>::print(std::ostream &os) const
{
    Base::print(os);
    os << " cl(" << *d()->cl << ")";
    os << " ghost(" << *d()->ghost << ")";
}

} // namespace vistle
#endif
