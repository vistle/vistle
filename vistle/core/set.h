#ifndef SET_H
#define SET_H

#include "shm.h"
#include "object.h"
#include <vector>

namespace vistle {

class Set: public Object {
   V_OBJECT(Set);

 public:
   typedef Object Base;

   struct Info: public Base::Info {
      std::vector<Object::Info *> items;
   };

   Set(const size_t numElements = 0,
         const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   Object::const_ptr getElement(const size_t index) const;
   void setElement(const size_t index, Object::const_ptr obj);
   void addElement(Object::const_ptr obj);
   shm<boost::interprocess::offset_ptr<Object::Data> >::vector &elements() const { return *(*d()->elements)(); }

 protected:
   struct Data: public Base::Data {

      ShmVector<boost::interprocess::offset_ptr<Object::Data> >::ptr elements;

      Data(const size_t numElements, const std::string & name,
            const int block, const int timestep);
      ~Data();
      static Data *create(const size_t numElements = 0,
            const int block = -1, const int timestep = -1);

      private:
      friend class Set;
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
         //ar & V_NAME("elements", *elements);
      }
   };
};


} // namespace vistle
#endif
