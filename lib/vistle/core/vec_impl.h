#ifndef VEC_IMPL_H
#define VEC_IMPL_H

namespace vistle {

#define V_IMPL_COMMA ,
template<class T, int Dim>
V_OBJECT_IMPL_LOAD(Vec<T V_IMPL_COMMA Dim>)

template<class T, int Dim>
V_OBJECT_IMPL_SAVE(Vec<T V_IMPL_COMMA Dim>)

template<class T, int Dim>
template<class Archive>
void Vec<T, Dim>::Data::serialize(Archive &ar)
{
    ar &V_NAME(ar, "base_object", serialize_base<Base::Data>(ar, *this));
    int dim = Dim;
    ar &V_NAME(ar, "dim", dim);
    assert(dim == Dim);
    for (int c = 0; c < Dim; ++c) {
        ar &V_NAME(ar, std::string("x" + std::to_string(c)).c_str(), x[c]);
    }
}

#undef V_IMPL_COMMA

} // namespace vistle

#endif
