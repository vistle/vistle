#ifndef INDEXED_IMPL_H
#define INDEXED_IMPL_H

namespace vistle {

template<class Archive>
void Indexed::Data::serialize(Archive &ar, const unsigned int version)
{
   ar & V_NAME("base_coords", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("element_list", *el);
   ar & V_NAME("connection_list", *cl);
}

template<typename Scalar, typename Index>
Index findCell(Indexed::const_ptr ind, typename Celltree<Scalar, Index>::const_ptr ct, Vector point) {

   return findCell<Scalar, Index>(ind, ct->nodes().data(), point, 0 /* current node */);
}

} // namespace vistle

#endif

