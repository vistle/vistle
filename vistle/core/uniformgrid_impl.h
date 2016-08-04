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

   for (int c=0; c<3; c++) {
       std::string nvpTag = "ghostLayers" + std::to_string(c);
       ar & V_NAME(nvpTag.c_str(), ghostLayers[c]);
   }
}

} // namespace vistle

#endif /* UNIFORM_GRID_IMPL_H */
