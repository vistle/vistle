#ifndef LINES_H
#define LINES_H

#include "scalar.h"
#include "shm.h"
#include "indexed.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT Lines: public Indexed {
   V_OBJECT(Lines);

 public:
   typedef Indexed Base;

   Lines(const size_t numElements, const size_t numCorners,
         const size_t numVertices,
         const int block = -1, const int timestep = -1);

   V_DATA_BEGIN(Lines);
      Data(const size_t numElements = 0, const size_t numCorners = 0,
         const size_t numVertices = 0, const std::string & name = "",
         const int block = -1, const int timestep = -1);
      static Data *create(const size_t numElements = 0, const size_t numCorners = 0,
         const size_t numVertices = 0,
         const int block = -1, const int timestep = -1);
   V_DATA_END(Lines);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "lines_impl.h"
#endif
#endif
