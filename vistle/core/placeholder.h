#ifndef PLACEHOLDER_H
#define PLACEHOLDER_H

#include "shm.h"
#include "object.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT PlaceHolder: public Object {
   V_OBJECT(PlaceHolder);

 public:
   typedef Object Base;

   PlaceHolder(const std::string &originalName, const Meta &originalMeta);

   void setReal(Object::const_ptr g);
   Object::const_ptr real() const;

   V_DATA_BEGIN(PlaceHolder);
      boost::interprocess::offset_ptr<Object::Data> real;

      Data(const std::string & name, const std::string &originalName,
            const Meta &m=Meta());
      ~Data();
      static Data *create();
      static Data *create(const std::string &originalName, const Meta &originalMeta);

      private:
      shm_name_t originalName;
      Meta originalMeta;
   V_DATA_END(PlaceHolder);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "placeholder_impl.h"
#endif
#endif
