#ifndef VISTLE_DATABASE_H
#define VISTLE_DATABASE_H

#include "index.h"
#include "dimensions.h"
#include "shm.h"
#include "object.h"
#include "vector.h"
#include "celltree.h"


namespace vistle {

class V_COREEXPORT DataBase: public Object {
   V_OBJECT(DataBase);

public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Mapping, (Unspecified)(Vertex)(Element));

   typedef Object Base;
   virtual Index getSize() const;
   virtual void setSize(const Index size);
   Object::const_ptr grid() const;
   void setGrid(Object::const_ptr grid);
   bool hasCelltree() const;
   Object::const_ptr getCelltree() const;
   Mapping guessMapping(Object::const_ptr grid=Object::const_ptr()) const; //< if Unspecified, try to derive a mapping based on array and grid size
   Mapping mapping() const;
   void setMapping(Mapping m);

   void copyAttributes(Object::const_ptr src, bool replace = true) override;

private:
   virtual void createCelltree(Index nelem, const Index *el, const Index *cl) const;

   V_DATA_BEGIN(DataBase);
      shm_obj_ref<Object> grid;
      Mapping mapping;

      Data(const Data &o, const std::string & name, Type id);
      ~Data();
      static Data *create(Type id = UNKNOWN, const Meta &meta=Meta());
   V_DATA_END(DataBase);
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(DataBase)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "database_impl.h"
#endif
#endif
