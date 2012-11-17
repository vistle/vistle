#ifndef GEOMETRY_IMPL_H
#define GEOMETRY_IMPL_H

namespace vistle {

template<class Archive>
void Geometry::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));

   bool haveGeometry = geometry;
   ar & V_NAME("haveGeometry", haveGeometry);
   if (haveGeometry)
      ar & V_NAME("geometry", *geometry);

   bool haveColors = colors;
   ar & V_NAME("haveColors", haveColors);
   if (haveColors)
      ar & V_NAME("colors", *colors);

   bool haveNormals = normals;
   ar & V_NAME("haveNormals", haveNormals);
   if (haveNormals)
      ar & V_NAME("normals", *normals);

   bool haveTexture = texture;
   ar & V_NAME("haveTexture", haveTexture);
   if (haveTexture)
      ar & V_NAME("texture", *texture);
}

} // namespace vistle

#endif
