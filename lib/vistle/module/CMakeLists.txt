set(module_SOURCES module.cpp objectcache.cpp reader.cpp resultcache.cpp)

set(module_HEADERS
    export.h
    module.h
    module_impl.h
    objectcache.h
    reader.h
    resultcache.h
    resultcache_impl.h)

if(NOT VISTLE_MODULES_SHARED)
    set(module_HEADERS ${module_HEADERS} moduleregistry.h)
    set(module_SOURCES ${module_SOURCES} moduleregistry.cpp)
endif()

use_openmp()

vistle_add_library(vistle_module EXPORT ${VISTLE_LIB_TYPE} ${module_SOURCES} ${module_HEADERS})
if(NOT VISTLE_MODULES_SHARED)
    target_link_libraries(vistle_module PRIVATE vistle_module_descriptions)
    target_link_libraries(vistle_module PRIVATE vistle_control)
endif()

if(UNIX AND NOT APPLE)
    vistle_target_link_libraries(vistle_module PRIVATE -lrt)
endif()

if(OpenMP_CXX_FOUND)
    vistle_target_link_libraries(vistle_module PUBLIC OpenMP::OpenMP_CXX)
endif()

vistle_target_link_libraries(
    vistle_module
    PUBLIC
    vistle_util
    PUBLIC
    vistle_util_mpi
    PUBLIC
    vistle_core
    PUBLIC
    vistle_alg
    PUBLIC
    ${BOOST_MPI}
    PUBLIC
    Boost::boost
    PUBLIC
    MPI::MPI_C)
