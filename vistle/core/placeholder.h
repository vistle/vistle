#ifndef PLACEHOLDER_H
#define PLACEHOLDER_H

#include "shmname.h"
#include "object.h"
#include "archives.h"
#include "shm_obj_ref.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT PlaceHolder: public Object {
   V_OBJECT(PlaceHolder);

 public:
   typedef Object Base;

   PlaceHolder(const std::string &originalName, const Meta &originalMeta, const Object::Type originalType);

   void setReal(Object::const_ptr g);
   Object::const_ptr real() const;
   std::string originalName() const;
   Object::Type originalType() const;
   const Meta &originalMeta() const;

   V_DATA_BEGIN(PlaceHolder);
      shm_obj_ref<Object> real;

      Data(const std::string & name, const std::string &originalName,
            const Meta &m, Object::Type originalType);
      ~Data();
      static Data *create(const std::string &name="", const std::string &originalName="", const Meta &originalMeta=Meta(), const Object::Type originalType=Object::UNKNOWN);

      private:
      shm_name_t originalName;
      Meta originalMeta;
      Object::Type originalType;
   V_DATA_END(PlaceHolder);
};

} // namespace vistle

V_OBJECT_DECLARE(vistle::PlaceHolder);
#endif

#ifdef VISTLE_IMPL
#include "placeholder_impl.h"
#endif
