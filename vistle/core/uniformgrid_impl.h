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
void UniformGrid::Data::serialize(Archive &ar) {
   ar & V_NAME(ar, "base_structuredGridBase", serialize_base<Base::Data>(ar, *this));
   ar & V_NAME(ar, "numElements", numDivisions);
   ar & V_NAME(ar, "min", min);
   ar & V_NAME(ar, "max", max);

   for (int c=0; c<3; c++) {
       std::string nvpTag = "ghostLayers" + std::to_string(c);
       ar & V_NAME(ar, nvpTag.c_str(), ghostLayers[c]);
   }
}

} // namespace vistle

#endif /* UNIFORM_GRID_IMPL_H */
