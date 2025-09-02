vistle_find_package(ZLIB)

include(FindPackageHandleStandardArgs)
set(EMBREE_MAJOR 4)
vistle_find_package(embree 4.0)
find_package_handle_standard_args(EMBREE DEFAULT_MSG EMBREE_LIBRARY EMBREE_INCLUDE_DIRS)
if(NOT EMBREE_FOUND)
    set(EMBREE_MAJOR 3)
    vistle_find_package(embree 3.0)
    find_package_handle_standard_args(EMBREE DEFAULT_MSG EMBREE_LIBRARY EMBREE_INCLUDE_DIRS)
endif()

include(CheckLanguage)
check_language(ISPC)
if(NOT EMBREE_FOUND)
    message("DisCOVERay: neither Embree 4 nor 3 found")
    return()
endif()

if(NOT CMAKE_ISPC_COMPILER)
    message("DisCOVERay: ISPC not found")
    return()
endif()

enable_language(ISPC)

set(CMAKE_ISPC_FLAGS "${CMAKE_ISPC_FLAGS} --wno-perf")
set(CMAKE_ISPC_FLAGS "${CMAKE_ISPC_FLAGS} --pic")

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -Wno-c99-extensions")
    set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -Wno-gnu-variable-sized-type-not-at-end")
endif()

set(USE_TBB FALSE)
if(OPENMP_FOUND)
    use_openmp()
elseif(TBB_FOUND)
    add_definitions(-DUSE_TBB)
    set(USE_TBB TRUE)
endif()

set(RAY_HEADERS common.h rayrenderobject.h)
set(RAY_SOURCES ray.cpp rayrenderobject.cpp)
set(ISPC_HEADERS render.isph renderobjectdata.isph spheres_isect.isph spheres.isph tubes.isph)
set(ISPC_SOURCES render.ispc renderobjectdata.ispc spheres.ispc tubes.ispc)
set(description "Embree CPU ray casting remote renderer")

add_compile_definitions(EMBREE_MAJOR=${EMBREE_MAJOR})
if(VISTLE_DOUBLE_PRECISION)
    add_compile_definitions(VISTLE_SCALAR_DOUBLE)
endif()

include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})
include_directories(SYSTEM ${EMBREE_INCLUDE_DIRS})
set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")

add_library(DisCOVERay_ispc OBJECT ${ISPC_HEADERS} ${ISPC_SOURCES})
add_module(DisCOVERay ${description} ${RAY_HEADERS} ${RAY_SOURCES})

target_link_libraries(
    DisCOVERay
    PRIVATE DisCOVERay_ispc
            ${BOOST_MPI}
            vistle_module
            vistle_renderer
            vistle_rhr
            vistle_config
            ${ZLIB_LIBRARIES}
            ${EMBREE_LIBRARY})

if(USE_LIBOMP AND LIBOMP)
    target_link_libraries(DisCOVERay PRIVATE ${LIBOMP})
endif()

if(USE_TBB)
    target_link_libraries(DisCOVERay PRIVATE TBB::tbb)
endif()
