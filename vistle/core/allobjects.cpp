// fewer files compile faster
#ifndef TEMPLATES_IN_HEADERS

#define VISTLE_IMPL
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
#include "placeholder.cpp"
#include "celltree.cpp"
#include "vertexownerlist.cpp"
#include "unstr.cpp"
#include "structuredgridbase.cpp"
#include "uniformgrid.cpp"
#include "rectilineargrid.cpp"
#include "structuredgrid.cpp"
#include "shmvectoroarchive.cpp"

#else

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
#include "placeholder.h"
#include "celltree.h"
#include "celltree_impl.h"
#include "vertexownerlist.h"
#include "unstr.h"
#include "structuredgridbase.h"
#include "uniformgrid.h"
#include "rectilineargrid.h"
#include "structuredgrid.h"
#include "shmvectoroarchive.h"

#endif

namespace {
   using namespace vistle;

   class RegisterObjectTypeRelations {
      public:
         RegisterObjectTypeRelations() {

            boost::serialization::void_cast_register<DataBase, DataBase::Base>
               (static_cast<DataBase *>(NULL), static_cast<DataBase::Base *>(NULL));

            boost::serialization::void_cast_register<Coords, Coords::Base>
               (static_cast<Coords *>(NULL), static_cast<Coords::Base *>(NULL));

            boost::serialization::void_cast_register<CoordsWithRadius, CoordsWithRadius::Base>
               (static_cast<CoordsWithRadius *>(NULL), static_cast<CoordsWithRadius::Base *>(NULL));

            boost::serialization::void_cast_register<Indexed, Indexed::Base>
               (static_cast<Indexed *>(NULL), static_cast<Indexed::Base *>(NULL));

#if 0
            boost::serialization::void_cast_register<StructuredGridBase, StructuredGridBase::Base>
               (static_cast<StructuredGridBase *>(NULL), static_cast<StructuredGridBase::Base *>(NULL));
#endif
         }
   };
   static RegisterObjectTypeRelations registerObjectTypeRelations;
}

namespace vistle {

void registerTypes() {

#ifdef VISTLE_STATIC
   using namespace vistle;
   REGISTER_TYPE(PlaceHolder, Object::PLACEHOLDER);
   REGISTER_TYPE(Texture1D, Object::TEXTURE1D);
   REGISTER_TYPE(Points, Object::POINTS);
   REGISTER_TYPE(Spheres, Object::SPHERES);
   REGISTER_TYPE(Lines, Object::LINES);
   REGISTER_TYPE(Tubes, Object::TUBES);
   REGISTER_TYPE(Triangles, Object::TRIANGLES);
   REGISTER_TYPE(Polygons, Object::POLYGONS);
   REGISTER_TYPE(UniformGrid, Object::UNIFORMGRID);
   REGISTER_TYPE(UniformGrid, Object::RECTILINEARGRID);
   REGISTER_TYPE(StructuredGrid, Object::STRUCTUREDGRID);
   REGISTER_TYPE(UnstructuredGrid, Object::UNSTRUCTUREDGRID);
   REGISTER_TYPE(VertexOwnerList, Object::VERTEXOWNERLIST)
   REGISTER_TYPE(Celltree1, Object::CELLTREE1);
   REGISTER_TYPE(Celltree2, Object::CELLTREE2);
   REGISTER_TYPE(Celltree3, Object::CELLTREE3);
   REGISTER_TYPE(Normals, Object::NORMALS);

   typedef Vec<unsigned char,1> Vec_uchar_1;
   REGISTER_TYPE(Vec_uchar_1, Vec_uchar_1::type());
   typedef Vec<unsigned char,3> Vec_uchar_3;
   REGISTER_TYPE(Vec_uchar_3, Vec_uchar_3::type());

   typedef Vec<int,1> Vec_int_1;
   REGISTER_TYPE(Vec_int_1, Vec_int_1::type());
   typedef Vec<int,3> Vec_int_3;
   REGISTER_TYPE(Vec_int_3, Vec_int_3::type());

   typedef Vec<unsigned int,1> Vec_uint_1;
   REGISTER_TYPE(Vec_uint_1, Vec_uint_1::type());
   typedef Vec<unsigned int,3> Vec_uint_3;
   REGISTER_TYPE(Vec_uint_3, Vec_uint_3::type());

   typedef Vec<size_t,1> Vec_szt_1;
   REGISTER_TYPE(Vec_szt_1, Vec_szt_1::type());
   typedef Vec<size_t,3> Vec_szt_3;
   REGISTER_TYPE(Vec_szt_3, Vec_szt_3::type());

   typedef Vec<float,1> Vec_float_1;
   REGISTER_TYPE(Vec_float_1, Vec_float_1::type());
   typedef Vec<float,3> Vec_float_3;
   REGISTER_TYPE(Vec_float_3, Vec_float_3::type());

   typedef Vec<double,1> Vec_double_1;
   REGISTER_TYPE(Vec_double_1, Vec_double_1::type());
   typedef Vec<double,3> Vec_double_3;
   REGISTER_TYPE(Vec_double_3, Vec_double_3::type());
#endif
}

} // namespace vistle
