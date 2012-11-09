#ifndef GEOMETRY_IMPL_H
#define GEOMETRY_IMPL_H

namespace vistle {

template<class Archive>
void Geometry::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("geometry", *geometry);
   ar & V_NAME("colors", *colors);
   ar & V_NAME("normals", *normals);
   ar & V_NAME("texture", *texture);
}

} // namespace vistle

#endif
