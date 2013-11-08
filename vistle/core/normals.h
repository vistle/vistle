#ifndef NORMALS_H
#define NORMALS_H


#include "coords.h"

namespace vistle {

class  V_COREEXPORT Normals: public Coords {
   V_OBJECT(Normals);

   public:
   typedef Coords Base;

   Normals(const Index numNormals,
         const Meta &meta=Meta());

   Index getNumNormals() const;

   V_DATA_BEGIN(Normals);

      Data(const Index numNormals = 0,
            const std::string & name = "",
            const Meta &meta=Meta());
      static Data *create(const Index numNormals = 0,
            const Meta &meta=Meta());

   V_DATA_END(Normals);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "normals_impl.h"
#endif

#endif
