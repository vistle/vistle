//-------------------------------------------------------------------------
// UNIFORM GRID CLASS IMPL H
// *
// * Uniform Grid Container Object
//-------------------------------------------------------------------------
#ifndef UNIFORM_GRID_IMPL_H
#define UNIFORM_GRID_IMPL_H

namespace vistle {

// BOOST SERIALIZATION METHOD
//-------------------------------------------------------------------------
template<class Archive>
void UniformGrid::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base_structuredGridBase", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("numElements", numDivisions);
   ar & V_NAME("min", min);
   ar & V_NAME("max", max);
}

} // namespace vistle

#endif /* UNIFORM_GRID_IMPL_H */
