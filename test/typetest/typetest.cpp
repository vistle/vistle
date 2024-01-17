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
#include <vistle/core/points.h>

using namespace vistle;

using namespace vistle;

int main(int argc, char *argv[])
{
    vistle::registerTypes();

    std::string shmname = "vistle_typetest";
    vistle::Shm::create(shmname, 1, 0, true);

    Vec<Index, 1>::ptr vi1(new Vec<Index, 1>(10));
    Vec<Index, 3>::ptr vi3(new Vec<Index, 3>(10));
    Vec<Scalar, 1>::ptr vs1(new Vec<Scalar, 1>(10));
    Vec<Scalar, 3>::ptr vs3(new Vec<Scalar, 3>(10));
    Vec<unsigned char, 1>::ptr vc1(new Vec<unsigned char, 1>(10));
    Vec<unsigned char, 3>::ptr vc3(new Vec<unsigned char, 3>(10));
    Vec<uint64_t, 1>::ptr vst1(new Vec<uint64_t, 1>(10));
    Vec<uint64_t, 3>::ptr vst3(new Vec<uint64_t, 3>(10));
    Points::ptr p(new Points(10));

#define P(v) std::cerr << #v << ": " << v->type() << " " << (int)(v->type()) << std::endl

    P(vi1);
    P(vi3);
    P(vs1);
    P(vs3);
    P(vc1);
    P(vc3);
    P(vst1);
    P(vst3);
    P(p);

    vistle::Shm::remove(shmname, 0, 0, true);

    return 0;
}
