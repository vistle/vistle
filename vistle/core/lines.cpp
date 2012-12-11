#include "lines.h"

namespace vistle {

Lines::Lines(const size_t numElements, const size_t numCorners,
                      const size_t numVertices,
                      const int block, const int timestep)
   : Lines::Base(Lines::Data::create(numElements, numCorners,
            numVertices, block, timestep))
{
}

Lines::Data::Data(const size_t numElements, const size_t numCorners,
             const size_t numVertices, const std::string & name,
             const int block, const int timestep)
   : Lines::Base::Data(numElements, numCorners, numVertices,
         Object::LINES, name, block, timestep)
{
}


Lines::Data * Lines::Data::create(const size_t numElements, const size_t numCorners,
                      const size_t numVertices,
                      const int block, const int timestep) {

   const std::string name = Shm::the().createObjectID();
   Data *l = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, block, timestep);
   publish(l);

   return l;
}

V_OBJECT_TYPE(Lines, Object::LINES);

} // namespace vistle
