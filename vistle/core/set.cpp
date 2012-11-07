#include "set.h"
#include "shm.h"

using namespace boost::interprocess;

namespace vistle {

Set::Set(const size_t numElements,
                  const int block, const int timestep)
: Set::Base(Set::Data::create(numElements, block, timestep))
{
}

Set::Data::Data(const size_t numElements, const std::string & name,
         const int block, const int timestep)
   : Set::Base::Data(Object::SET, name, block, timestep)
     , elements(new ShmVector<offset_ptr<Object::Data> >(numElements))
{
}

Set::Data::~Data() {

   for (size_t i = 0; i < elements->size(); ++i) {

      if ((*elements)[i]) {
         (*elements)[i]->unref();
      }
   }
}


Set::Data * Set::Data::Data::create(const size_t numElements,
                  const int block, const int timestep) {

   const std::string name = Shm::the().createObjectID();
   Data *p = shm<Data>::construct(name)(numElements, name, block, timestep);
   publish(p);

   return p;
}

size_t Set::getNumElements() const {

   return d()->elements->size();
}

Object::const_ptr Set::getElement(const size_t index) const {

   if (index >= d()->elements->size())
      return Object::ptr();

   return Object::create((*d()->elements)[index].get());
}

void Set::setElement(const size_t index, Object::const_ptr object) {

   if (index >= d()->elements->size()) {

      std::cerr << "WARNING: automatically resisizing set" << std::endl;
      d()->elements->resize(index+1);
   }

   if (Object::const_ptr old = getElement(index)) {
      old->unref();
   }

   if (object) {
      object->ref();
      (*d()->elements)[index] = object->d();
   } else {
      (*d()->elements)[index] = NULL;
   }
}

void Set::addElement(Object::const_ptr object) {

   if (object) {
      object->ref();
      d()->elements->push_back(object->d());
   }
}

V_OBJECT_TYPE(Set, Object::SET);

} // namespace vistle
