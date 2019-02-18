// fewer files compile faster
#ifndef TEMPLATES_IN_HEADERS

#define VISTLE_IMPL
#include "archives.h"
#include "vec.cpp"
#include "coords.cpp"
#include "coordswradius.cpp"
#include "normals.cpp"
#include "points.cpp"
#include "spheres.cpp"
#include "tubes.cpp"
#include "indexed.cpp"
#include "lines.cpp"
#include "triangles.cpp"
#include "polygons.cpp"
#include "texture1d.cpp"
#include "empty.cpp"
#include "placeholder.cpp"
#include "celltree.cpp"
#include "vertexownerlist.cpp"
#include "unstr.cpp"
#include "structuredgridbase.cpp"
#include "uniformgrid.cpp"
#include "rectilineargrid.cpp"
#include "structuredgrid.cpp"
#include "findobjectreferenceoarchive.cpp"

#else

#include "object.h"
#include "empty.h"
#include "archives.h"
#include "vec.h"
#include "coords.h"
#include "coordswradius.h"
#include "normals.h"
#include "points.h"
#include "spheres.h"
#include "tubes.h"
#include "indexed.h"
#include "lines.h"
#include "triangles.h"
#include "polygons.h"
#include "texture1d.h"
#include "empty.h"
#include "placeholder.h"
#include "celltree.h"
#include "celltree_impl.h"
#include "vertexownerlist.h"
#include "unstr.h"
#include "structuredgridbase.h"
#include "uniformgrid.h"
#include "rectilineargrid.h"
#include "structuredgrid.h"
#include "findobjectreferenceoarchive.h"

#endif

namespace vistle {

#define REGISTER_TYPE(ObjType, id) \
{ \
   ObjectTypeRegistry::registerType<ObjType>(id); \
}

#define REGISTER_VEC_TYPE(t, d) \
{ \
   ObjectTypeRegistry::registerType<Vec<t,d>>(Vec<t,d>::type()); \
}

void registerTypes() {

   using namespace vistle;
   REGISTER_TYPE(Empty, Object::EMPTY);
   REGISTER_TYPE(PlaceHolder, Object::PLACEHOLDER);
   REGISTER_TYPE(Texture1D, Object::TEXTURE1D);
   REGISTER_TYPE(Points, Object::POINTS);
   REGISTER_TYPE(Spheres, Object::SPHERES);
   REGISTER_TYPE(Lines, Object::LINES);
   REGISTER_TYPE(Tubes, Object::TUBES);
   REGISTER_TYPE(Triangles, Object::TRIANGLES);
   REGISTER_TYPE(Polygons, Object::POLYGONS);
   REGISTER_TYPE(UniformGrid, Object::UNIFORMGRID);
   REGISTER_TYPE(RectilinearGrid, Object::RECTILINEARGRID);
   REGISTER_TYPE(StructuredGrid, Object::STRUCTUREDGRID);
   REGISTER_TYPE(UnstructuredGrid, Object::UNSTRUCTUREDGRID);
   REGISTER_TYPE(VertexOwnerList, Object::VERTEXOWNERLIST)
   REGISTER_TYPE(Celltree1, Object::CELLTREE1);
   REGISTER_TYPE(Celltree2, Object::CELLTREE2);
   REGISTER_TYPE(Celltree3, Object::CELLTREE3);
   REGISTER_TYPE(Normals, Object::NORMALS);

   REGISTER_VEC_TYPE(unsigned char, 1);
   REGISTER_VEC_TYPE(unsigned char, 3);
   REGISTER_VEC_TYPE(int32_t, 1);
   REGISTER_VEC_TYPE(int32_t, 3);
   REGISTER_VEC_TYPE(uint32_t, 1);
   REGISTER_VEC_TYPE(uint32_t, 3);
   REGISTER_VEC_TYPE(int64_t, 1);
   REGISTER_VEC_TYPE(int64_t, 3);
   REGISTER_VEC_TYPE(uint64_t, 1);
   REGISTER_VEC_TYPE(uint64_t, 3);
   REGISTER_VEC_TYPE(float, 1);
   REGISTER_VEC_TYPE(float, 3);
   REGISTER_VEC_TYPE(double, 1);
   REGISTER_VEC_TYPE(double, 3);
}

} // namespace vistle
