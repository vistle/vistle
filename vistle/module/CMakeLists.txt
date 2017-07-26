set(module_SOURCES
   module.cpp
   objectcache.cpp
)

set(module_HEADERS
   export.h
   module.h
   module_impl.h
   objectcache.h
)

if (NOT VISTLE_MODULES_SHARED)
   set(module_HEADERS ${module_HEADERS} moduleregistry.h)
   set(module_SOURCES ${module_SOURCES} moduleregistry.cpp)
endif()

use_openmp()

vistle_add_library(vistle_module ${VISTLE_LIB_TYPE} ${module_SOURCES} ${module_HEADERS})

if(UNIX AND NOT APPLE)
	target_link_libraries(vistle_module
		-lrt
	)
endif()

target_link_libraries(vistle_module
        vistle_util
        vistle_core
        vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ..
)


add_subdirectory(test)
add_subdirectory(general)
