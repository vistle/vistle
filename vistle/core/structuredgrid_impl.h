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
void StructuredGrid::Data::serialize(Archive &ar) {
   ar & V_NAME(ar, "base_structuredGridBase", serialize_base<Base::Data>(ar, *this));
   ar & V_NAME(ar, "num_divisions", numDivisions);
   ar & V_NAME(ar, "coords_x", x[0]);
   ar & V_NAME(ar, "coords_y", x[1]);
   ar & V_NAME(ar, "coords_z", x[2]);

   for (int c=0; c<3; c++) {
       std::string nvpTag = "ghostLayers" + std::to_string(c);
       ar & V_NAME(ar, nvpTag.c_str(), ghostLayers[c]);
   }
}

} // namespace vistle

#endif /* STRUCTURED_GRID_IMPL_H */
