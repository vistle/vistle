//-------------------------------------------------------------------------
// STRUCTURED GRID CLASS IMPL H
// *
// * Structured Grid Container Object
//-------------------------------------------------------------------------
#ifndef STRUCTURED_GRID_IMPL_H
#define STRUCTURED_GRID_IMPL_H

namespace vistle {

// BOOST SERIALIZATION METHOD
//-------------------------------------------------------------------------
template<class Archive>
void StructuredGrid::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base_structuredGridBase", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("num_divisions", numDivisions);
   ar & V_NAME("coords_x", x[0]);
   ar & V_NAME("coords_y", x[1]);
   ar & V_NAME("coords_z", x[2]);

   for (int c=0; c<3; c++) {
       std::string nvpTag = "ghostLayers" + std::to_string(c);
       ar & V_NAME(nvpTag.c_str(), ghostLayers[c]);
   }
}

} // namespace vistle

#endif /* STRUCTURED_GRID_IMPL_H */
