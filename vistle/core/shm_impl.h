#ifndef SHM_IMPL_H
#define SHM_IMPL_H

#include <boost/type_traits.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/push_back.hpp>
#include <iostream>

#include "scalars.h"
#include "celltreenode.h"

//#include "archives.h"

namespace vistle {

typedef boost::mpl::push_back<Scalars, CelltreeNode<sizeof(Index), 1>>::type VectorTypes1;
typedef boost::mpl::push_back<VectorTypes1, CelltreeNode<sizeof(Index), 2>>::type VectorTypes2;
typedef boost::mpl::push_back<VectorTypes2, CelltreeNode<sizeof(Index), 3>>::type VectorTypes;

template<typename T, class allocator>
int shm_array<T, allocator>::typeId() {
   const size_t pos = boost::mpl::find<VectorTypes, T>::type::pos::value;
   BOOST_STATIC_ASSERT(pos < boost::mpl::size<VectorTypes>::value);
   return pos;
}

template<typename T>
const ShmVector<T> Shm::getArrayFromName(const std::string &name) const {
   typedef vistle::shm_array<T, typename vistle::shm<T>::allocator> array;

   auto arr = vistle::shm<array>::find(name);

   if (!arr || array::typeId() != arr->type()) {
       return vistle::shm_ref<array>();
   }

   return vistle::shm_ref<array>(name, arr);
}

template<class Archive>
void shm_name_t::serialize(Archive &ar, const unsigned int version) {

   boost::serialization::split_member(ar, *this, version);
}

template<class Archive>
void shm_name_t::save(Archive &ar, const unsigned int version) const {

   std::string n(name.data());
   //std::cerr << "SHM_NAME_T save: '" << n << "'" << std::endl;
   ar & boost::serialization::make_nvp("shm_name_t", n);
}

template<class Archive>
void shm_name_t::load(Archive &ar, const unsigned int version) {

   std::string n;
   ar & boost::serialization::make_nvp("shm_name_t", n);
   auto end = n.find('\0');
   if (end != std::string::npos) {
      n = n.substr(0, end);
   }
   if (n.size() < name.size()) {
      std::copy(n.begin(), n.end(), name.data());
      name[n.size()] = '\0';
   } else {
      std::cerr << "shm_name_t: name too long: " << n << " (" << n.size() << " chars)" << std::endl;
      memset(name.data(), 0, name.size());
      assert(n.size() < name.size());
   }
   //std::cerr << "SHM_NAME_T load: '" << name.data() << "'" << std::endl;
}

} // namespace vistle

#endif
