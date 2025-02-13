#ifndef VISTLE_CORE_SHM_CONFIG_H
#define VISTLE_CORE_SHM_CONFIG_H

#ifdef NO_SHMEM
#include "shmdata.h"
#else
#ifdef _WIN32
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#else
#include <boost/interprocess/managed_shared_memory.hpp>
#endif
#endif


namespace vistle {

#ifdef NO_SHMEM
typedef ShmData *shm_handle_t;
template<typename T>
using shm_allocator = vistle::default_init_allocator<T>;
#else
#ifdef _WIN32
typedef boost::interprocess::managed_windows_shared_memory managed_shm;
#else
typedef boost::interprocess::managed_shared_memory managed_shm;
#endif

typedef managed_shm::handle_t shm_handle_t;
template<typename T>
using shm_allocator = boost::interprocess::allocator<T, managed_shm::segment_manager>;
#endif

} // namespace vistle
#endif
