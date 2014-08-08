#ifndef EMPTY_H
#define EMPTY_H

#include "shm.h"
#include "object.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT Empty: public Object {
   V_OBJECT(Empty);

 public:
   typedef Object Base;

   Empty(const std::string &originalName);

   V_DATA_BEGIN(Empty);

      Data(const std::string & name,
            const Meta &m);
      ~Data();
      static Data *create();

      private:
    V_DATA_END(Empty);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "empty_impl.h"
#endif
#endif
