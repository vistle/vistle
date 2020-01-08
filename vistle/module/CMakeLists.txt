set(module_SOURCES
   module.cpp
   objectcache.cpp
   reader.cpp
)

set(module_HEADERS
   export.h
   module.h
   module_impl.h
   objectcache.h
   reader.h
)

if (NOT VISTLE_MODULES_SHARED)
   set(module_HEADERS ${module_HEADERS} moduleregistry.h)
   set(module_SOURCES ${module_SOURCES} moduleregistry.cpp)
endif()

use_openmp()

vistle_add_library(vistle_module ${VISTLE_LIB_TYPE} ${module_SOURCES} ${module_HEADERS})

if(UNIX AND NOT APPLE)
	target_link_libraries(vistle_module
        PRIVATE -lrt
	)
endif()

if (OpenMP_CXX_FOUND)
    target_link_libraries(vistle_module PUBLIC OpenMP::OpenMP_CXX)
endif()

target_link_libraries(vistle_module
    PUBLIC vistle_util
    PUBLIC vistle_core
    PUBLIC vistle_boost_mpi
    PUBLIC ${Boost_LIBRARIES}
    PUBLIC ${MPI_C_LIBRARIES}
)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
include_directories(
        ..
)


add_subdirectory(test)
add_subdirectory(general)

if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/vistle/module/univiz" AND EXISTS ${PROJECT_SOURCE_DIR}/vistle/module/univiz/CMakeLists.txt)
    set(UNIVIZ_VISTLE TRUE)
    add_subdirectory(univiz)
endif()
