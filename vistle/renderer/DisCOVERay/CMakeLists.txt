include(FindPackageHandleStandardArgs)
vistle_find_package(embree 2.0)
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

SET(XEON_ISA "AVX512" CACHE STRING "Selects highest ISA to support.")
SET_PROPERTY(CACHE XEON_ISA PROPERTY STRINGS SSE2 SSE3 SSSE3 SSE4.1 SSE4.2 AVX AVX-I AVX2 AVX512)

IF (XEON_ISA STREQUAL "SSE2")
  SET(ISA  1)
ENDIF ()

IF (XEON_ISA STREQUAL "SSE3")
  SET(ISA  2)
ENDIF ()

IF (XEON_ISA STREQUAL "SSSE3")
  SET(ISA  3)
ENDIF ()

IF (XEON_ISA STREQUAL "SSE4.1")
  SET(ISA  4)
ENDIF ()

IF (XEON_ISA STREQUAL "SSE4.2")
  SET(ISA  5)
ENDIF ()

IF (XEON_ISA STREQUAL "AVX")
  SET(ISA  6)
ENDIF ()

IF (XEON_ISA STREQUAL "AVX-I")
  SET(ISA  7)
ENDIF ()

IF (XEON_ISA STREQUAL "AVX2")
  SET(ISA  8)
ENDIF ()

IF (XEON_ISA STREQUAL "AVX512")
  SET(ISA  9)
ENDIF ()

SET(TARGET_SSE2  OFF)
SET(TARGET_SSE3  OFF)
SET(TARGET_SSSE3  OFF)
SET(TARGET_SSE41  OFF)
SET(TARGET_SSE42  OFF)
SET(TARGET_AVX  OFF)
SET(TARGET_AVXI  OFF)
SET(TARGET_AVX2  OFF)
SET(TARGET_AVX512 OFF)

IF (ISA GREATER 0)
  SET(TARGET_SSE2  ON)
  SET(ISPC_TARGETS "sse2")
ENDIF ()

IF (ISA GREATER 1)
  SET(TARGET_SSE3  ON)
ENDIF ()

IF (ISA GREATER 2)
  SET(TARGET_SSSE3  ON)
ENDIF ()

IF (ISA GREATER 3)
  SET(TARGET_SSE41  ON)
  LIST(APPEND ISPC_TARGETS "sse4")
ENDIF ()

IF (ISA GREATER 4)
  SET(TARGET_SSE42  ON)
ENDIF ()

IF (ISA GREATER 5)
  SET(TARGET_AVX  ON)
  LIST(APPEND ISPC_TARGETS "avx")
ENDIF ()

IF (ISA GREATER 6)
  SET(TARGET_AVXI  ON)
ENDIF ()

IF (ISA GREATER 7)
  SET(TARGET_AVX2  ON)
  LIST(APPEND ISPC_TARGETS "avx2")
ENDIF ()

IF (ISA GREATER 8)
  SET(TARGET_AVX512  ON)
  LIST(APPEND ISPC_TARGETS "avx512skx-i32x16")
ENDIF ()

set(__XEON__ 1)
set(HAVE_ISPC ON)
set(ENABLE_ISPC_SUPPORT ON)
include (ispc)
include_directories_ispc(${EMBREE_INCLUDE_DIRS})
# END: ispc

if (ENABLE_ISPC_SUPPORT)
   set(ALL_MODULES ${ALL_MODULES} DisCOVERay CACHE INTERNAL "")
   add_definitions(-DVISTLE_MODULE_NAME="DisCOVERay")
   add_definitions(-DHAVE_ISPC)
   if ((CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo") OR (CMAKE_BUILD_TYPE STREQUAL "Debug"))
      set(ISPC_COMPILE_FLAGS "-g")
   endif()
   if(VISTLE_MULTI_PROCESS)
      add_ispc_executable(DisCOVERay rayrenderobject.cpp ray.cpp common.h  rayrenderobject.h
         render.ispc renderobjectdata.ispc spheres.ispc tubes.ispc)
      set_target_properties(DisCOVERay PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/libexec/module)
      install(TARGETS DisCOVERay RUNTIME DESTINATION libexec/module)
   else()
      if (VISTLE_MODULES_SHARED)
         set(type MODULE)
      else()
         set(type STATIC)
      endif()
      add_ispc_library(DisCOVERay ${type} rayrenderobject.cpp ray.cpp common.h  rayrenderobject.h
         render.ispc renderobjectdata.ispc spheres.ispc tubes.ispc)
      set_target_properties(DisCOVERay PROPERTIES
         RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/module
         LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/module)
      if (VISTLE_MODULES_SHARED)
         install(TARGETS DisCOVERay LIBRARY DESTINATION lib/module)
      endif()
   endif()
else()
   add_module(DisCOVERay rayrenderobject.cpp ray.cpp render.ispc common.h  rayrenderobject.h)
endif()
add_custom_target(ispc SOURCES render.ispc renderobjectdata.ispc spheres.ispc tubes.ispc render.isph  renderobjectdata.isph  spheres_isect.isph  spheres.isph  tubes.isph)


include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${ICET_INCLUDE_DIRS}
        ${EMBREE_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}
)

target_link_libraries(DisCOVERay
        ${Boost_LIBRARIES}
        vistle_boost_mpi
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_renderer
        vistle_rhr
        ${ICET_CORE_LIBS}
        ${ICET_MPI_LIBS}
        ${ZLIB_LIBRARIES}
        ${EMBREE_LIBRARY}
        ${TBB_LIBRARIES}
)

if (JPEGTURBO_FOUND AND TURBOJPEG_HAVE_TJMCUWIDTH)
   target_link_libraries(DisCOVERay ${JPEG_LIBRARIES})
endif()
