include(FindPackageHandleStandardArgs)
vistle_find_package(embree 3.0)
find_package_handle_standard_args(EMBREE DEFAULT_MSG EMBREE_LIBRARY EMBREE_INCLUDE_DIRS)

vistle_find_package(TBB)
vistle_find_package(ZLIB)
vistle_find_package(JPEGTURBO)

if (NOT TBB_FOUND)
   message("DisCOVERay: TBB not found")
   return()
endif()

if(NOT EMBREE_FOUND)
   message("DisCOVERay: Embree not found")
   return()
endif()

if(TBB_FOUND)
    #if (NOT OPENMP_FOUND)
      add_definitions(-DUSE_TBB)
      #endif()
else()
   use_openmp()
endif()

if(JPEGTURBO_FOUND)
   if(TURBOJPEG_HAVE_TJMCUWIDTH)
      add_definitions(-DHAVE_TURBOJPEG)
      include_directories(SYSTEM ${JPEGTURBO_INCLUDE_DIR})
   else()
      message("DisCOVERay: turbojpeg w/o tjMcuWidth")
   endif()
endif()

# BEGIN: ispc
##############################################################
# ISA configuration
##############################################################

IF (EMBREE_MAX_ISA STREQUAL "NONE")
   SET(MAX_ISA  0)
ENDIF()

IF (EMBREE_MAX_ISA STREQUAL "NONE")
   SET(XEON_ISA "AVX512" CACHE STRING "Selects ISA to support.")
else()
   SET(XEON_ISA "RANGE" CACHE STRING "Selects ISA to support.")
ENDIF ()
SET_PROPERTY(CACHE XEON_ISA PROPERTY STRINGS RANGE SSE2 SSE3 SSSE3 SSE4.1 SSE4.2 AVX AVX-I AVX2 AVX512)

IF (EMBREE_MAX_ISA STREQUAL "SSE2")
   SET(MAX_ISA  1)
ENDIF ()

IF (EMBREE_MAX_ISA STREQUAL "SSE3")
   SET(MAX_ISA  2)
ENDIF ()

IF (EMBREE_MAX_ISA STREQUAL "SSSE3")
   SET(MAX_ISA  3)
ENDIF ()

IF (EMBREE_MAX_ISA STREQUAL "SSE4.1")
   SET(MAX_ISA  4)
ENDIF ()

IF (EMBREE_MAX_ISA STREQUAL "SSE4.2")
   SET(MAX_ISA  5)
ENDIF ()

IF (EMBREE_MAX_ISA STREQUAL "AVX")
   SET(MAX_ISA  6)
ENDIF ()

IF (EMBREE_MAX_ISA STREQUAL "AVX-I")
   SET(MAX_ISA  7)
ENDIF ()

IF (EMBREE_MAX_ISA STREQUAL "AVX2")
   SET(MAX_ISA  8)
ENDIF ()

IF (EMBREE_MAX_ISA STREQUAL "AVX512")
   SET(MAX_ISA  9)
ENDIF ()

IF (MAX_ISA GREATER 0 OR EMBREE_ISA_SSE2)
  SET(ISPC_TARGETS "sse2")
ENDIF ()

IF (MAX_ISA GREATER 3 OR EMBREE_ISA_SSE42)
  LIST(APPEND ISPC_TARGETS "sse4")
ENDIF ()

IF (MAX_ISA GREATER 5 OR EMBREE_ISA_AVX)
  LIST(APPEND ISPC_TARGETS "avx")
ENDIF ()

IF (MAX_ISA GREATER 7 OR EMBREE_ISA_AVX2)
  LIST(APPEND ISPC_TARGETS "avx2")
ENDIF ()

IF (MAX_ISA GREATER 8 OR EMBREE_ISA_AVX512SKX)
  LIST(APPEND ISPC_TARGETS "avx512skx-i32x16")
ENDIF ()

set(__XEON__ 1)
set(HAVE_ISPC ON)
set(ENABLE_ISPC_SUPPORT ON)
include (ispc)
include_directories_ispc(${EMBREE_INCLUDE_DIRS})
# END: ispc

set(RAY_HEADERS common.h rayrenderobject.h)
set(RAY_SOURCES ray.cpp rayrenderobject.cpp)
set(ISPC_HEADERS 
    render.isph  renderobjectdata.isph
    spheres_isect.isph  spheres.isph  
    tubes.isph
)
set(ISPC_SOURCES 
        render.ispc renderobjectdata.ispc
        spheres.ispc
        tubes.ispc
)

set(ALL_MODULES ${ALL_MODULES} DisCOVERay CACHE INTERNAL "")
add_definitions(-DVISTLE_MODULE_NAME="DisCOVERay")
add_definitions(-DHAVE_ISPC)
if ((CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo") OR (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set(ISPC_COMPILE_FLAGS "-g")
endif()
if(VISTLE_MULTI_PROCESS)
    add_ispc_executable(DisCOVERay ${RAY_HEADERS} ${RAY_SOURCES} ${ISPC_SOURCES})
    set_target_properties(DisCOVERay PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/libexec/module)
    install(TARGETS DisCOVERay RUNTIME DESTINATION libexec/module)
else()
    if (VISTLE_MODULES_SHARED)
        set(type MODULE)
    else()
        set(type STATIC)
    endif()
    add_ispc_library(DisCOVERay ${type} ${RAY_HEADERS} ${RAY_SOURCES} ${ISPC_SOURCES})
    set_target_properties(DisCOVERay PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/module
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/module)
    if (VISTLE_MODULES_SHARED)
        install(TARGETS DisCOVERay LIBRARY DESTINATION lib/module)
    endif()
endif()
add_custom_target(ispc SOURCES ${ISPC_SOURCES} ${ISPC_HEADERS})

include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})
set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")

SET(EXTRA_LIBS ${EXTRA_LIBS} ${MPI_C_LIBRARIES})
ADD_DEFINITIONS(-DHAS_MPI)
include_directories(SYSTEM
        ${ICET_INCLUDE_DIRS}
        ${EMBREE_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}
)

target_link_libraries(DisCOVERay
        vistle_boost_mpi
        vistle_module
        vistle_renderer
        vistle_rhr
        ${ICET_CORE_LIBS}
        ${ZLIB_LIBRARIES}
        ${EMBREE_LIBRARY}
        ${TBB_LIBRARIES}
		${EXTRA_LIBS}
)

if (JPEGTURBO_FOUND AND TURBOJPEG_HAVE_TJMCUWIDTH)
   target_link_libraries(DisCOVERay ${JPEG_LIBRARIES})
endif()
