// fewer files compile faster

#define VISTLE_IMPL

#include "vec.cpp"
#include "coords.cpp"
#include "points.cpp"
#include "indexed.cpp"
#include "lines.cpp"
#include "triangles.cpp"
#include "polygons.cpp"
#include "unstr.cpp"
#include "geometry.cpp"
#include "texture1d.cpp"

namespace {
   using namespace vistle;

   class RegisterObjectTypeRelations {
      public:
         RegisterObjectTypeRelations() {
            boost::serialization::void_cast_register<Coords, Coords::Base>
               (static_cast<Coords *>(NULL), static_cast<Coords::Base *>(NULL));

            boost::serialization::void_cast_register<Indexed, Indexed::Base>
               (static_cast<Indexed *>(NULL), static_cast<Indexed::Base *>(NULL));
         }
   };
   static RegisterObjectTypeRelations registerObjectTypeRelations;
}
