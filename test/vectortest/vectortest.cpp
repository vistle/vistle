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

#include <mpi.h>

#include <vistle/core/shm.h>
#include <vistle/core/vec.h>
#include <vistle/core/shm_array_impl.h>

using namespace vistle;

//#define TWICE

struct Aligned {
    int v;

    Aligned() { std::cerr << "Aligned default ctor: " << v << std::endl; }

    Aligned(const Aligned &o): v(o.v) { std::cerr << "Aligned copy ctor: " << v << std::endl; }

    Aligned(Aligned &&o): v(o.v) { std::cerr << "Aligned move ctor: " << v << std::endl; }

    Aligned(int v): v(v) { std::cerr << "Aligned ctor: " << v << std::endl; }

    Aligned &operator=(const Aligned &rhs)
    {
        v = rhs.v;
        std::cerr << "Aligned assignment: " << v << std::endl;
        return *this;
    }

    Aligned &operator=(Aligned &&rhs)
    {
        v = rhs.v;
        std::cerr << "Aligned move assignment: " << v << std::endl;
        return *this;
    }

    bool check(int vv)
    {
        assert(v == vv);
        return v == vv;
    }

    ~Aligned() { std::cerr << "Aligned dtor: " << v << std::endl; }
};

struct Unaligned {
    int v1, v2, v3;
    double large;
    uint64_t large2;

    Unaligned(int v = 0): v1(v), v2(v), v3(v)
    {
        //std::cerr << "Unaligned ctor: " << v << std::endl;
    }

    ~Unaligned()
    {
        //std::cerr << "Unalgined dtor: " << v1 << ", " << v2 << ", " << v3 << std::endl;
    }

    bool check(int v)
    {
        assert(v == v1);
        assert(v == v2);
        assert(v == v3);
        return v == v1 && v == v2 && v == v3;
    }
};

template<class container>
void test_pb_int(container &v, const std::string &tag, int size)
{
#if 0
   typename container::value_type val, arr[10];
   std::cerr << "member size: " << sizeof(val) << ", aligned size: " << sizeof(arr)/10 << std::endl;
#endif

    clock_t start = clock();
    for (int i = 0; i < size; ++i) {
        v.push_back(i);
    }
    for (int i = 0; i < size; ++i) {
        if (v[i] != i) {
            std::cerr << tag << ": error at " << i << ", was " << v[i] << std::endl;
        }
    }
    clock_t elapsed = clock() - start;
    std::cerr << size << " " << tag << ": " << (double)elapsed / CLOCKS_PER_SEC << std::endl;
}

template<class container>
void test_pb(container &v, const std::string &tag, int size)
{
#if 0
   typename container::value_type val, arr[10];
   std::cerr << "member size: " << sizeof(val) << ", aligned size: " << sizeof(arr)/10 << std::endl;
#endif

    clock_t start = clock();
    for (int i = 0; i < size; ++i) {
        v.push_back(i);
    }
    for (int i = 0; i < size; ++i) {
        if (!v[i].check(i)) {
            std::cerr << tag << ": error at " << i << std::endl;
        }
    }
    clock_t elapsed = clock() - start;
    std::cerr << size << " " << tag << ": " << (double)elapsed / CLOCKS_PER_SEC << std::endl;
}

template<class container>
void test_eb(container &v, const std::string &tag, int size)
{
#if 0
   typename container::value_type val, arr[10];
   std::cerr << "member size: " << sizeof(val) << ", aligned size: " << sizeof(arr)/10 << std::endl;
#endif

    clock_t start = clock();
    for (int i = 0; i < size; ++i) {
        v.emplace_back(i);
    }
    for (int i = 0; i < size; ++i) {
        if (!v[i].check(i)) {
            std::cerr << tag << ": error at " << i << std::endl;
        }
    }
    clock_t elapsed = clock() - start;
    std::cerr << size << " " << tag << ": " << (double)elapsed / CLOCKS_PER_SEC << std::endl;
}

namespace bi = boost::interprocess;

using namespace vistle;

int main(int argc, char *argv[])
{
    vistle::registerTypes();

    std::string shmname = "vistle_vectortest";

    int shift = 10;
    if (argc > 1) {
        shift = atoi(argv[1]);
    }
    const Index size = 1L << shift;

#if 0
   {
      std::vector<int> v;
      test_pb_int(v, "STL vector int", size);
   }
   {
      std::vector<int> v;
      v.reserve(size);
      test_pb_int(v, "STL vector reserve", size);
   }
#endif
    {
        std::vector<Aligned> v;
        test_eb(v, "STL vector emplace_back", size);
    }
    {
        std::vector<Aligned> v;
        test_pb(v, "STL vector", size);
    }
    {
        std::vector<Aligned> v;
        v.reserve(size);
        test_pb(v, "STL vector reserve", size);
    }

    {
        bi::vector<int> v;
        test_pb_int(v, "B:I vector", size);
    }

    {
        vistle::shm_array<int, std::allocator<int>> v(std::allocator<int>{});
        test_pb_int(v, "uninit array", size);
    }

    {
        vistle::shm_array<int, std::allocator<int>> v(std::allocator<int>{});
        v.reserve(size);
        test_pb_int(v, "uninit array reserve", size);
    }

#if 0
   {
      vistle::shm_array<Aligned, std::allocator<Aligned>> v;
      test_eb(v, "uninit array emplace_back", size);
   }

   {
      vistle::shm_array<Aligned, std::allocator<Aligned>> v;
      test_pb(v, "uninit array", size);
   }

   {
      vistle::shm_array<Aligned, std::allocator<Aligned>> v;
      v.reserve(size);
      test_pb(v, "uninit array", size);
   }

   {
      vistle::shm_array<Unaligned, std::allocator<Unaligned>> v;
      test_pb(v, "uninit array reserve", size);
   }

   {
      vistle::shm_array<Unaligned, std::allocator<Unaligned>> v;
      v.reserve(size);
      test_pb(v, "uninit array reserve", size);
   }
#endif

#if 0
   { 
      bi::shared_memory_object::remove(shmname.c_str());
      vistle::Shm::create(shmname, 0, 0, NULL);
      vistle::Vec<Index, 1> v(Index(0));
      test_pb(v.x(), "vistle push_back only", size);
      bi::shared_memory_object::remove(shmname.c_str());
   }
   { 
      bi::shared_memory_object::remove(shmname.c_str());
      vistle::Shm::create(shmname, 0, 0, NULL);
      vistle::Vec<Index, 1> v(Index(0));
      v.x().reserve(size);
      test_pb(v.x(), "vistle push_back+reserve", size);
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
      test_pb(vv, "vistle push_back+reserve", size);
      bi::shared_memory_object::remove(shmname.c_str());
   }
#endif

#if 0
   { 
      bi::shared_memory_object::remove(shmname.c_str());
      vistle::Shm::create(shmname, 0, 0, NULL);
      {
         vistle::shm<Unaligned>::array_ptr vp = Shm::the().shm().construct<vistle::shm<Unaligned>::array>("testvector")(0, Shm::the().allocator());
         vistle::shm<Unaligned>::array &v = *vp;
         test_pb(v, "vistle explicit uninit", size);
      }
      bi::shared_memory_object::remove(shmname.c_str());
   }

   { 
      bi::shared_memory_object::remove(shmname.c_str());
      vistle::Shm::create(shmname, 0, 0, NULL);
      vistle::shm<Index>::ptr v7p = Shm::the().shm().construct<vistle::shm<Index>::vector>("testvector")(0, 0, Shm::the().allocator());
      vistle::shm<Index>::vector &v = *v7p;
      v.reserve(size);
      test_pb(v, "vistle explicit reserved+push_back", size);
      time_arr(v, "vistle explicit arr", size);
      time_ptr(&v[0], "vistle explicit arr ptr", size);
#ifdef TWICE
      time_arr(v, "vistle explicit arr", size);
      time_ptr(&v[0], "vistle explicit arr ptr", size);
#endif
      bi::shared_memory_object::remove(shmname.c_str());
   }
#endif

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
