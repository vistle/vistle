#include <memory>

#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <vistle/core/shm_array.h>
#include <vistle/core/shm.h>
#include <vistle/core/vec.h>

#include <vistle/util/allocator.h>

using namespace vistle;

typedef Index DataType;

struct DataClass {
    DataType v;

    DataClass(DataType v): v(v) {}
};

//#define TWICE

template<class container>
void time_resize(container &v, const std::string &tag, Index size)
{
    clock_t start = clock();
    v.resize(size);
    clock_t elapsed = clock() - start;
    std::cerr << size << " " << tag << ": " << (double)elapsed / CLOCKS_PER_SEC << std::endl;
}

template<class container>
void time_pb(container &v, const std::string &tag, Index size)
{
    clock_t start = clock();
    for (Index i = 0; i < size; ++i) {
        v.push_back(i);
    }
    clock_t elapsed = clock() - start;
    std::cerr << size << " " << tag << ": " << (double)elapsed / CLOCKS_PER_SEC << std::endl;
}

template<class container>
void time_eb(container &v, const std::string &tag, Index size)
{
    clock_t start = clock();
    for (Index i = 0; i < size; ++i) {
        v.emplace_back(i);
    }
    clock_t elapsed = clock() - start;
    std::cerr << size << " " << tag << ": " << (double)elapsed / CLOCKS_PER_SEC << std::endl;
}

template<class container>
void time_arr(container &v, const std::string &tag, Index size)
{
    clock_t start = clock();
    for (Index i = 0; i < size; ++i) {
        v[i] = i;
    }
    clock_t elapsed = clock() - start;
    std::cerr << size << " " << tag << ": " << (double)elapsed / CLOCKS_PER_SEC << std::endl;
}

template<class T>
void time_ptr(T *v, const std::string &tag, Index size)
{
    clock_t start = clock();
    for (Index i = 0; i < size; ++i) {
        v[i] = i;
    }
    clock_t elapsed = clock() - start;
    std::cerr << size << " " << tag << ": " << (double)elapsed / CLOCKS_PER_SEC << std::endl;
}

template<typename T>
static T min(T a, T b)
{
    return a < b ? a : b;
}

template<class V>
void time_all(const std::string &name, size_t size)
{
    {
        V v;
        time_resize(v, name + " resize", size);
    }
    {
        V v;
        time_pb(v, name + " push_back", size);
    }
    {
        V v;
        v.reserve(size);
        time_pb(v, name + " reserve+push_back", size);
        time_arr(v, name + " arr", size);
        time_ptr(&v[0], name + " arr ptr", size);
#ifdef TWICE
        time_arr(v, name + " arr", size);
        time_ptr(&v[0], name + " arr ptr", size);
#endif
    }
    {
        V v;
        v.reserve(size);
        time_eb(v, name + " emplace_back+reserve", size);
    }
}

namespace bi = boost::interprocess;

using namespace vistle;

int main(int argc, char *argv[])
{
    vistle::registerTypes();

    std::string shmname = "vistle_vectortest";

    int shift = 24;
    if (argc > 1) {
        shift = atoi(argv[1]);
    }
    const Index size = 1L << shift;

    time_all<std::vector<DataType>>("STL vector", size);
    time_all<std::vector<DataType, vistle::default_init_allocator<DataType>>>("STL vector+default_init", size);
    time_all<bi::vector<DataType>>("Boost vector", size);
    //time_all<bi::vector<DataType, vistle::default_init_allocator,DataType>>>("Boost vector+default_init", size);
    time_all<vistle::shm_array<DataType, std::allocator<DataType>>>("shm_array", size);
    //time_all<vistle::shm_array<DataType, vistle::default_init_allocator<DataType>>>("shm_array+default_init", size);

#if 0
   {
      vistle::shm_array<DataType, std::allocator<DataType>> v;
      v.reserve(size);
      time_eb(v, "uninit array reserve+emplace_back", size);
   }

   {
      vistle::shm_array<DataClass, std::allocator<DataClass>> v;
      v.reserve(size);
      time_pb(v, "uninit array push_back+reserve class", size);
      time_arr(v, "uninit array arr class", size);
      time_ptr(&v[0], "uninit array arr ptr class", size);
#ifdef TWICE
      time_arr(v, "uninit array arr", size);
      time_ptr(&v[0], "uninit array arr ptr", size);
#endif
   }
   {
      vistle::shm_array<DataClass, std::allocator<DataClass>> v;
      v.reserve(size);
      time_eb(v, "uninit array reserve+emplace_back class", size);
   }
#endif

    {
        bi::shared_memory_object::remove(shmname.c_str());
        vistle::Shm::create(shmname, 1, 0, true);
        vistle::Vec<DataType, 1> v(size_t(0));
        time_pb(v.x(), "vistle push_back only", size);
        bi::shared_memory_object::remove(shmname.c_str());
    }
    {
        bi::shared_memory_object::remove(shmname.c_str());
        vistle::Shm::create(shmname, 1, 0, true);
        vistle::Vec<DataType, 1> v(size_t(0));
        v.x().reserve(size);
        time_pb(v.x(), "vistle push_back+reserve", size);
        time_arr(v.x(), "vistle vector arr", size);
        time_ptr(&(v.x()[0]), "vistle vector arr ptr", size);
        time_arr(v.x(), "vistle vector arr", size);
#ifdef TWICE
        time_ptr(&(v.x()[0]), "vistle vector arr ptr", size);
        bi::shared_memory_object::remove(shmname.c_str());
#endif
        vistle::Shm::remove(shmname, 1, 0, true);
    }

    {
        bi::shared_memory_object::remove(shmname.c_str());
        vistle::Shm::create(shmname, 1, 0, true);
        vistle::Vec<DataType, 1> v(size_t(0));
        shm<DataType>::array &vv = v.x();
        vv.reserve(size);
        time_pb(vv, "vistle push_back+reserve", size);
        bi::shared_memory_object::remove(shmname.c_str());
        vistle::Shm::remove(shmname, 1, 0, true);
    }

    {
        bi::shared_memory_object::remove(shmname.c_str());
        vistle::Shm::create(shmname, 1, 0, true);
        vistle::shm<DataType>::array_ptr v7p =
            Shm::the().shm().construct<vistle::shm<DataType>::array>("testvector")(0, Shm::the().allocator());
        vistle::shm<DataType>::array &v = *v7p;
        v.reserve(size);
        time_pb(v, "vistle explicit uninit reserved+push_back", size);
        time_arr(v, "vistle explicit uninit arr", size);
        time_ptr(&v[0], "vistle explicit uninit arr ptr", size);
#ifdef TWICE
        time_arr(v, "vistle explicit uninit arr", size);
        time_ptr(&v[0], "vistle explicit arr ptr", size);
#endif
        bi::shared_memory_object::remove(shmname.c_str());
        vistle::Shm::remove(shmname, 1, 0, true);
    }

#if 0
   { 
      bi::shared_memory_object::remove(shmname.c_str());
      vistle::Shm::create(shmname, 1, 0, true);
      auto v7p = Shm::the().shm().construct<vistle::shm<DataType>::vector>("testvector")(0, 0, Shm::the().allocator());
      vistle::shm<DataType>::vector &v = *v7p;
      v.reserve(size);
      time_pb(v, "vistle explicit reserved+push_back", size);
      time_arr(v, "vistle explicit arr", size);
      time_ptr(&v[0], "vistle explicit arr ptr", size);
#ifdef TWICE
      time_arr(v, "vistle explicit arr", size);
      time_ptr(&v[0], "vistle explicit arr ptr", size);
#endif
      bi::shared_memory_object::remove(shmname.c_str());
      vistle::Shm::remove(shmname, 1, 0, true);
   }
#endif

#if 0
   bi::shared_memory_object::remove(shmname.c_str());
   vistle::Shm::create(shmname, 1, 0, true);
   start = clock();
   vistle::Vec<DataType, 1> v7(DataType(0));
   auto v7p = &(v7.x()());
   for (DataType i=0; i<size; ++i) {
      v7p->push_back(i);
   }
   clock_t vi_dir = clock()-start;
   std::cerr << size << " vistle direct push_back: " << (double)vi_dir/CLOCKS_PER_SEC << std::endl;
   bi::shared_memory_object::remove(shmname.c_str());
   vistle::Shm::remove(shmname, 1, 0, true);
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
