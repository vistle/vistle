//-------------------------------------------------------------------------
// RECTILINEAR GRID CLASS IMPL H
// *
// * Rectilinear Grid Container Object
//-------------------------------------------------------------------------
#ifndef RECTILINEAR_GRID_IMPL_H
#define RECTILINEAR_GRID_IMPL_H

namespace vistle {

// BOOST SERIALIZATION METHOD
//-------------------------------------------------------------------------
template<class Archive>
void RectilinearGrid::Data::serialize(Archive &ar) {
   ar & V_NAME(ar, "base_structuredGridBase", serialize_base<Base::Data>(ar, *this));
   ar & V_NAME(ar, "coords_x", coords[0]);
   ar & V_NAME(ar, "coords_y", coords[1]);
   ar & V_NAME(ar, "coords_z", coords[2]);

   for (int c=0; c<3; c++) {
       std::string nvpTag = "ghostLayers" + std::to_string(c);
       ar & V_NAME(ar, nvpTag.c_str(), ghostLayers[c]);
   }
}

} // namespace vistle

#endif /* RECTILINEAR_GRID_IMPL_H */
