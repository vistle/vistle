#include "empty.h"
#include "empty_impl.h"

namespace vistle {

bool Empty::isEmpty() const {

   return true;
}

bool Empty::checkImpl() const {

   return true;
}

Empty::Data::Data(const Empty::Data &o, const std::string &n)
: Empty::Base::Data(o, n)
{

}

Empty::Data::Data(const std::string & name,
      const Meta &m)
: Empty::Base::Data(Object::EMPTY, name, m)

{
}

Empty::Data::~Data() {
}

Empty::Data * Empty::Data::create() {

   const std::string name = Shm::the().createObjectID();
   Data *p = shm<Data>::construct(name)(name, Meta());
   publish(p);

   return p;
}


V_OBJECT_TYPE(Empty, Object::EMPTY);

} // namespace vistle
