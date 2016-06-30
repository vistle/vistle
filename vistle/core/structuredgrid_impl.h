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
   ar & V_NAME("numElements", numElements);
   ar & V_NAME("coords_x", coords_x);
   ar & V_NAME("coords_y", coords_y);
   ar & V_NAME("coords_z", coords_z);
}

} // namespace vistle

#endif /* STRUCTURED_GRID_IMPL_H */
