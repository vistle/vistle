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
void RectilinearGrid::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base_structuredGridBase", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("coords_x", coords[0]);
   ar & V_NAME("coords_y", coords[1]);
   ar & V_NAME("coords_z", coords[2]);
}

} // namespace vistle

#endif /* RECTILINEAR_GRID_IMPL_H */
