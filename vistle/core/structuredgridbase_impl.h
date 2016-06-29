//-------------------------------------------------------------------------
// STRUCTURED GRID OBJECT BASE CLASS IMPL H
// *
// * Base class for Structured Grid Objects
//-------------------------------------------------------------------------
#ifndef STRUCTURED_GRID_BASE_IMPL_H
#define STRUCTURED_GRID_BASE_IMPL_H

namespace vistle {

// BOOST SERIALIZATION METHOD
//-------------------------------------------------------------------------
template<class Archive>
void StructuredGridBase::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base_obj", boost::serialization::base_object<Base::Data>(*this));
}

} // namespace vistle

#endif /* STRUCTURED_GRID_BASE_IMPL_H */
