#include "lines.h"

namespace vistle {

Lines::Lines(const size_t numElements, const size_t numCorners,
                      const size_t numVertices,
                      const Meta &meta)
   : Lines::Base(Lines::Data::create(numElements, numCorners,
            numVertices, meta))
{
}

Lines::Data::Data(const Data &other, const std::string &name)
: Lines::Base::Data(other, name)
{
}

Lines::Data::Data(const size_t numElements, const size_t numCorners,
             const size_t numVertices, const std::string & name,
             const Meta &meta)
   : Lines::Base::Data(numElements, numCorners, numVertices,
         Object::LINES, name, meta)
{
}


Lines::Data * Lines::Data::create(const size_t numElements, const size_t numCorners,
                      const size_t numVertices,
                      const Meta &meta) {

   const std::string name = Shm::the().createObjectID();
   Data *l = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, meta);
   publish(l);

   return l;
}

V_OBJECT_TYPE(Lines, Object::LINES);

} // namespace vistle
