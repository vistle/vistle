#include "lines.h"

namespace vistle {

Lines::Lines(const Index numElements, const Index numCorners,
                      const Index numVertices,
                      const Meta &meta)
   : Lines::Base(Lines::Data::create("", numElements, numCorners,
            numVertices, meta))
{
}

bool Lines::isEmpty() const {

   return Base::isEmpty();
}

bool Lines::checkImpl() const {

   return true;
}

Lines::Data::Data(const Data &other, const std::string &name)
: Lines::Base::Data(other, name)
{
}

Lines::Data::Data(const Index numElements, const Index numCorners,
             const Index numVertices, const std::string & name,
             const Meta &meta)
   : Lines::Base::Data(numElements, numCorners, numVertices,
         Object::LINES, name, meta)
{
}


Lines::Data * Lines::Data::create(const std::string &objId, const Index numElements, const Index numCorners,
                      const Index numVertices,
                      const Meta &meta) {

   const std::string name = Shm::the().createObjectId(objId);
   Data *l = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, meta);
   publish(l);

   return l;
}

V_OBJECT_TYPE(Lines, Object::LINES);

} // namespace vistle
