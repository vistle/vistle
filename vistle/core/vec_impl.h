#ifndef VEC_IMPL_H
#define VEC_IMPL_H

namespace vistle {

template <class T, int Dim>
Vec<T,Dim>::Vec(const size_t size,
        const int block, const int timestep)
      : Object(Data::create(size, block, timestep)) {
   }

template <class T, int Dim>
void Vec<T,Dim>::setSize(const size_t size) {
   for (int c=0; c<Dim; ++c)
      d()->x[c]->resize(size);
}

template <class T, int Dim>
template <class Archive>
void Vec<T,Dim>::Data::serialize(Archive &ar, const unsigned int version) {

   ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
   int dim = Dim;
   ar & V_NAME("dim", dim);
   assert(dim == Dim);
   for (int c=0; c<Dim; ++c)
      ar & V_NAME("x", *x[c]);
}

} // namespace vistle

#endif
