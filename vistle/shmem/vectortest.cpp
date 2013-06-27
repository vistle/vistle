#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>

#include <iostream>
#include <string>

#include <mpi.h>

#include <core/shm.h>
#include <core/vec.h>

using namespace vistle;

//#define TWICE

template<class container>
void time_pb(container &v, const std::string &tag, Index size) {

   clock_t start = clock();
   for (Index i=0; i<size; ++i) {
      v.push_back(i);
   }
   clock_t elapsed = clock()-start;
   std::cerr << size << " " << tag << ": " << (double)elapsed/CLOCKS_PER_SEC << std::endl;
}

template<class container>
void time_arr(container &v, const std::string &tag, Index size) {

   clock_t start = clock();
   for (Index i=0; i<size; ++i) {
      v[i] = i;
   }
   clock_t elapsed = clock()-start;
   std::cerr << size << " " << tag << ": " << (double)elapsed/CLOCKS_PER_SEC << std::endl;
}

template<class T>
void time_ptr(T *v, const std::string &tag, Index size) {

   clock_t start = clock();
   for (Index i=0; i<size; ++i) {
      v[i] = i;
   }
   clock_t elapsed = clock()-start;
   std::cerr << size << " " << tag << ": " << (double)elapsed/CLOCKS_PER_SEC << std::endl;
}

template<typename T>
static T min(T a, T b) { return a<b ? a : b; }

//namespace bi = boost::interprocess;

using namespace vistle;

int main(int argc, char *argv[]) {

   std::string shmname = "vistle_vectortest";

   int shift = 24;
   if (argc > 1) {
      shift = atoi(argv[1]);
   }
   const Index size = 1L << shift;

   { 
      std::vector<Index> v;
      time_pb(v, "STL vector push_back", size);
   }
   {
      std::vector<Index> v;
      v.reserve(size);
      time_pb(v, "STL vector reserve+push_back", size);
      time_arr(v, "STL vector arr", size);
      time_ptr(&v[0], "STL vector arr ptr", size);
#ifdef TWICE
      time_arr(v, "STL vector arr", size);
      time_ptr(&v[0], "STL vector arr ptr", size);
#endif
   }

   { 
      bi::vector<Index> v;
      time_pb(v, "B:I vector push_back", size);
   }
   {
      bi::vector<Index> v;
      v.reserve(size);
      time_pb(v, "B:I vector reserve+push_back", size);
      time_arr(v, "B:I vector arr", size);
      time_ptr(&v[0], "B:I vector arr ptr", size);
#ifdef TWICE
      time_arr(v, "B:I vector arr", size);
      time_ptr(&v[0], "B:I vector arr ptr", size);
#endif
   }

   {
      vistle::shm_array<Index, std::allocator<Index>> v;
      time_pb(v, "uninit array push_back only", size);
      time_arr(v, "uninit array arr", size);
      time_ptr(&v[0], "uninit array arr ptr", size);
#ifdef TWICE
      time_arr(v, "uninit array arr", size);
      time_ptr(&v[0], "uninit array arr ptr", size);
#endif
   }

   {
      vistle::shm_array<Index, std::allocator<Index>> v;
      v.reserve(size);
      time_pb(v, "uninit array reserve+push_back", size);
      time_arr(v, "uninit array arr", size);
      time_ptr(&v[0], "uninit array arr ptr", size);
#ifdef TWICE
      time_arr(v, "uninit array arr", size);
      time_ptr(&v[0], "uninit array arr ptr", size);
#endif
   }

   { 
      bi::shared_memory_object::remove(shmname.c_str());
      vistle::Shm::create(shmname, 0, 0, NULL);
      vistle::Vec<Index, 1> v(Index(0));
      time_pb(v.x(), "vistle push_back only", size);
      bi::shared_memory_object::remove(shmname.c_str());
   }
   { 
      bi::shared_memory_object::remove(shmname.c_str());
      vistle::Shm::create(shmname, 0, 0, NULL);
      vistle::Vec<Index, 1> v(Index(0));
      v.x().reserve(size);
      time_pb(v.x(), "vistle push_back+reserve", size);
      time_arr(v.x(), "vistle vector arr", size);
      time_ptr(&(v.x()[0]), "vistle vector arr ptr", size);
      time_arr(v.x(), "vistle vector arr", size);
#ifdef TWICE
      time_ptr(&(v.x()[0]), "vistle vector arr ptr", size);
      bi::shared_memory_object::remove(shmname.c_str());
#endif
   }

   { 
      bi::shared_memory_object::remove(shmname.c_str());
      vistle::Shm::create(shmname, 0, 0, NULL);
      vistle::Vec<Index, 1> v(Index(0));
      shm<Index>::array &vv = v.x();
      vv.reserve(size);
      time_pb(vv, "vistle push_back+reserve", size);
      bi::shared_memory_object::remove(shmname.c_str());
   }

   { 
      bi::shared_memory_object::remove(shmname.c_str());
      vistle::Shm::create(shmname, 0, 0, NULL);
      vistle::shm<Index>::ptr v7p = Shm::the().shm().construct<vistle::shm<Index>::vector>("testvector")(size, 0, Shm::the().allocator());
      vistle::shm<Index>::vector &v = *v7p;
      v.reserve(size);
      time_pb(v, "vistle explicit reserved+push_back", size);
      time_arr(v, "vistle explicit arr", size);
      time_ptr(&v[0], "vistle explicit arr ptr", size);
      time_arr(v, "vistle explicit arr", size);
#ifdef TWICE
      time_ptr(&v[0], "vistle explicit arr ptr", size);
      bi::shared_memory_object::remove(shmname.c_str());
#endif
   }

#if 0
   bi::shared_memory_object::remove(shmname.c_str());
   vistle::Shm::create(shmname, 0, 0, NULL);
   start = clock();
   vistle::Vec<Index, 1> v7(Index(0));
   auto v7p = &(v7.x()());
   for (Index i=0; i<size; ++i) {
      v7p->push_back(i);
   }
   clock_t vi_dir = clock()-start;
   std::cerr << size << " vistle direct push_back: " << (double)vi_dir/CLOCKS_PER_SEC << std::endl;
   bi::shared_memory_object::remove(shmname.c_str());
#endif

#if 0 
   managed_shared_memory *shm = NULL;
   try {
      shm = new managed_shared_memory(create_only, "/test", 1L<<shift);
   } catch (std::exception &ex) {
      std::cerr << "exception: " << ex.what() << std::endl;
      MPI_Finalize();
      exit(1);
   }

   if (shm) {
      std::cerr << "success" << std::endl;
   } else {
      std::cerr << "failure" << std::endl;
   }

   MPI_Finalize();
#endif

   return 0;
}

