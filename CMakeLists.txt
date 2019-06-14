cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_C_STANDARD 99)
set(CMAKE_CXX_EXTENSIONS OFF)

project(Vistle C CXX)

#find_package(Threads)
#message("THREADS: ${CMAKE_THREAD_LIB_INIT}")

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "CrayLinuxEnvironment")
   message("compiling for Cray")
   set(CRAY ON)
   #set(Boost_DEBUG ON)
   set(Boost_NO_BOOST_CMAKE OFF)
   set(CRAYPE_LINK_TYPE "$ENV{CRAYPE_LINK_TYPE}")
   if (NOT "${CRAYPE_LINK_TYPE}" STREQUAL "dynamic")
      message("dynamic linking required: set CRAYPE_LINK_TYPE to \"dynamic\"")
      return()
   endif()

   set(VISTLE_USE_PYTHON3 ON CACHE INTERNAL "")

   # work around broken FindCUDA
   macro( find_host_program )
      set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
      set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER )
      set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER )
      if( CMAKE_HOST_WIN32 )
         SET( WIN32 1 )
         SET( UNIX )
      elseif( CMAKE_HOST_APPLE )
         SET( APPLE 1 )
         SET( UNIX )
      endif()
      find_program( ${ARGN} )
      SET( WIN32 )
      SET( APPLE )
      SET( UNIX 1 )
      set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY )
      set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
      set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
   endmacro()
endif()

if(POLICY CMP0042)
   # default to finding shlibs relative to @rpath on MacOS
   cmake_policy(SET CMP0042 NEW)
endif()

# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

if (POLICY CMP0074)
   # make find_include/find_library search in <PackageName>_ROOT prefix
   cmake_policy(SET CMP0074 NEW)
endif()

# vim's YouCompleteMe can make use of this
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

option(VISTLE_BUILD_SHARED "Build all libraries as shared libraries instead of static" ON)
set(VISTLE_CPU_ARCH "corei7-avx" CACHE STRING "CPU architecture to optimize for (native, core-avx2, atom, ...)")

option(VISTLE_SANITIZE_UNDEFINED "Build with -fsanitize=undefined" OFF)
option(VISTLE_SANITIZE_ADDRESS "Build with -fsanitize=address" OFF)
option(VISTLE_SANITIZE_THREAD "Build with -fsanitize=thread" OFF)

option(VISTLE_USE_CUDA "Build with CUDA support" OFF)

if (NOT "$ENV{VBUILD}" STREQUAL "")
   include("${PROJECT_SOURCE_DIR}/build/$ENV{VBUILD}.cmake")
endif()

if (VISTLE_BUILD_SHARED)
   set(VISTLE_LIB_TYPE SHARED)
else()
   set(VISTLE_LIB_TYPE STATIC)
   add_definitions(-DVISTLE_STATIC)
   set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
endif()

# allow for optimizations in Boost.MPI
if (NOT WIN32)
   add_definitions(-DBOOST_MPI_HOMOGENEOUS)
endif()

enable_testing()

if (CMAKE_BUILD_TYPE STREQUAL "")
   set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
      "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
      FORCE
   )
   message("Setting CMAKE_BUILD_TYPE to ${CMAKE_BUILD_TYPE}")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
   # disable rpath - only rely on LD_LIBRARY_PATH and the likes
   set(CMAKE_SKIP_RPATH TRUE)
else()
   add_definitions(-DEIGEN_INITIALIZE_MATRICES_BY_NAN)
   set(CMAKE_SKIP_RPATH FALSE)
endif()

#add_definitions(-DEIGEN_DEFAULT_DENSE_INDEX_TYPE=short)
add_definitions(-DEIGEN_DEFAULT_DENSE_INDEX_TYPE=int)

# use this instead of FIND_PACKAGE to prefer Package in $PACKAGE_HOME and $EXTERNLIBS/package
macro(vistle_find_package package)
   set(SAVED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})

   set(pack "${package}")
   if (pack STREQUAL "PythonLibs")
      set(pack "Python")
   endif()
   if (pack STREQUAL "PythonInterp")
      set(pack "Python")
   endif()

   string(TOUPPER ${pack} UPPER)
   string(TOLOWER ${pack} LOWER)
   if(MINGW)
      set(CMAKE_PREFIX_PATH ${MINGW_SYSROOT} ${CMAKE_PREFIX_PATH})
   endif()
   if(NOT "$ENV{EXTERNLIBS}" STREQUAL "")
      set(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS}/${LOWER}/bin ${CMAKE_PREFIX_PATH})
      set(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS} ${CMAKE_PREFIX_PATH})
      set(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS}/${LOWER} ${CMAKE_PREFIX_PATH})
   endif()
   if(NOT "$ENV{${UPPER}_HOME}" STREQUAL "")
      set(CMAKE_PREFIX_PATH $ENV{${UPPER}_HOME} ${CMAKE_PREFIX_PATH})
   endif()
   if(NOT "$ENV{${UPPER}_DIR}" STREQUAL "")
      set(CMAKE_PREFIX_PATH $ENV{${UPPER}_DIR} ${CMAKE_PREFIX_PATH})
   endif()
   if(NOT "$ENV{${UPPER}ROOT}" STREQUAL "")
      set(CMAKE_PREFIX_PATH $ENV{${UPPER}ROOT} ${CMAKE_PREFIX_PATH})
   endif()
   if(NOT "$ENV{${UPPER}_ROOT}" STREQUAL "")
      set(CMAKE_PREFIX_PATH $ENV{${UPPER}_ROOT} ${CMAKE_PREFIX_PATH})
   endif()
   #message("looking for package ${ARGV}")
   #message("CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
   find_package(${ARGV})

   set(CMAKE_PREFIX_PATH ${SAVED_CMAKE_PREFIX_PATH})
endmacro(vistle_find_package PACKAGE)

# ensure that OpenMP works with Apple Clang and libomp installed
if (APPLE)
    set(SAVED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    set(CMAKE_PREFIX_PATH /usr/local/opt/libomp ${CMAKE_PREFIX_PATH})
    find_path(LIBOMP_PREFIX include/omp.h)
    find_path(LIBOMP_PREFIX0 lib/libomp.dylib)
    set(CMAKE_PREFIX_PATH ${SAVED_CMAKE_PREFIX_PATH})
    if (LIBOMP_PREFIX AND LIBOMP_PREFIX0 AND (LIBOMP_PREFIX STREQUAL LIBOMP_PREFIX0))
        message("OpenMP: Found libomp - enabling with Apple Clang")
        set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include" CACHE STRING "OpenMP C flags")
        set(OpenMP_CXX_FLAGS ${OpenMP_C_FLAGS} CACHE STRING "OpenMP CXX flags")
        set(OpenMP_C_LIB_NAMES omp CACHE STRING "OpenMP C library")
        set(OpenMP_CXX_LIB_NAMES ${OpenMP_C_LIB_NAMES} CACHE STRING "OpenMP CXX library")
        set(OpenMP_omp_LIBRARY ${LIBOMP_PREFIX}/lib/libomp.dylib CACHE STRING "OpenMP library")
    endif()
endif()
find_package(OpenMP)

option(VISTLE_USE_PYTHON3 "use Python 3" ON)
if(VISTLE_USE_PYTHON3)
   set(PythonLibs_FIND_VERSION_MAJOR 3)
   message("Searching for Python 3")
else()
   if (NOT CMAKE_VERSION VERSION_LESS 3.6)
      set(PythonLibs_FIND_VERSION 2.7 2.6)
   endif()
   set(PythonLibs_FIND_VERSION_MAJOR 2)
   message("Searching for Python 2")
endif()
vistle_find_package(PythonLibs REQUIRED)
if (UNIX AND NOT APPLE)
    set(PYTHON_LIBRARIES ${PYTHON_LIBRARIES} ${CMAKE_DL_LIBS})
    set(PYTHON_LIBRARIES ${PYTHON_LIBRARIES} -lutil)
endif()

if(WIN32)
   #set(BOOST_ROOT "$ENV{EXTERNLIBS}/boost")
   #STRING(REGEX REPLACE "\\\\" "/" BOOST_ROOT ${BOOST_ROOT}) 
    #set(MPI_HOME "$ENV{EXTERNLIBS}/OpenMPI")
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/vistle/cmake/Modules/windows;${CMAKE_MODULE_PATH}")
    #add_definitions(-DOMPI_IMPORTS)

    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)

    add_definitions(-DBOOST_ALL_NO_LIB)
    #add_definitions(-DBOOST_ALL_DYN_LINK)
    add_definitions(-DBOOST_MPI_DYN_LINK)
    add_definitions(-DBOOST_PROGRAM_OPTIONS_DYN_LINK)
    add_definitions(-DBOOST_ZLIB_DYN_LINK)
    add_definitions(-DBOOST_IOSTREAMS_DYN_LINK)

    add_definitions(-D_WIN32_WINNT=0x0601) # windows 7
    set(MSVC_WARNINGS "${MSVC_WARNINGS} /wd4244") # possible loss of data
    set(MSVC_WARNINGS "${MSVC_WARNINGS} /wd4250") # 'class1': inheritcs 'class2::member' via dominance
    set(MSVC_WARNINGS "${MSVC_WARNINGS} /wd4251") # 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
    set(MSVC_WARNINGS "${MSVC_WARNINGS} /wd4267") # 'var' : conversion from 'size_t' to 'type', possible loss of data
    set(MSVC_WARNINGS "${MSVC_WARNINGS} /wd4275") # non - DLL-interface class 'class_1' used as base for DLL-interface class 'class_2'
    set(MSVC_WARNINGS "${MSVC_WARNINGS} /wd4661") # 'identifier' : no suitable definition provided for explicit template instantiation request
    set(MSVC_WARNINGS "${MSVC_WARNINGS} /wd4910") # '<identifier>' : '__declspec(dllexport)' and 'extern' are incompatible on an explicit instantiation
    set(MSVC_WARNINGS "${MSVC_WARNINGS} /wd4996") # The compiler encountered a deprecated declaration.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_WARNINGS} /EHsc")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MSVC_WARNINGS} /EHsc")
endif(WIN32)

#vistle_find_package(IceT REQUIRED)
set(ICET_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/3rdparty/IceT/src/include;${CMAKE_BINARY_DIR}/3rdparty/IceT/src/include)
set(ICET_CORE_LIBS "IceTCore")
set(ICET_MPI_LIBS "IceTMPI")
set(ICET_FOUND TRUE)

set(ASAN_COMPILE_FLAGS "")
set(ASAN_LINK_FLAGS "")
if (VISTLE_SANITIZE_UNDEFINED)
   set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=undefined")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=undefined")
endif()
if (VISTLE_SANITIZE_ADDRESS)
   set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=address")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=address")
endif()
if (VISTLE_SANITIZE_THREAD)
   set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=thread")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=thread")
endif()
if (VISTLE_SANITIZE_THREAD OR VISTLE_SANITIZE_ADDRESS OR VISTLE_SANITIZE_UNDEFINED)
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fno-omit-frame-pointer")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -g3")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fPIC")
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_COMPILE_FLAGS}")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} ${ASAN_LINK_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${ASAN_LINK_FLAGS}")

set (BOOST_INCLUDEDIR /usr/include/boost169)  
set (BOOST_LIBRARYDIR /usr/lib64/boost169)
vistle_find_package(Boost 1.53 REQUIRED COMPONENTS system)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-command-line-argument")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

   if (NOT ${BUILD_SHARED_LIBS})
      # for linking the libraries into plugins
      #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
      #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
      set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
   endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
endif()

if (NOT CRAY)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(APPLE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wno-stdlibcxx-not-found")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

    if (DEFINED VISTLE_CPU_ARCH)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${VISTLE_CPU_ARCH}")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${VISTLE_CPU_ARCH}")
    endif()
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    if (NOT APPLE)
      if (DEFINED VISTLE_CPU_ARCH)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${VISTLE_CPU_ARCH}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${VISTLE_CPU_ARCH}")
      endif()
    endif()

    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
      set(CUDA_HOST_COMPILER ${CMAKE_SOURCE_DIR}/scripts/cuda-host-compiler CACHE STRING "CUDA nvcc host compiler" FORCE)
    endif()

  endif()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
#elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif()

if (NOT CRAY)
   set(MPI_SKIP_MPICXX TRUE)
   vistle_find_package(MPI REQUIRED COMPONENTS C CXX)

   include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})
   set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS}")
   set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")
endif()

add_definitions(-DBOOST_LIB_DIAGNOSTIC=1)
if (Boost_VERSION VERSION_GREATER 106099)
   set(BOOST_MPI_DIR "boost-mpi")
elseif (Boost_VERSION VERSION_GREATER 105899)
   set(BOOST_MPI_DIR "boost-mpi-1.60")
elseif (Boost_VERSION VERSION_GREATER 105599)
   set(BOOST_MPI_DIR "boost-mpi-1.58")
else()
   set(BOOST_MPI_DIR "boost-mpi-1.55")
endif()
#message("Using Boost.MPI from ${BOOST_MPI_DIR} for Boost ${Boost_VERSION}")

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if (LIB64)
   set(LIBSUFFIX 64)
else()
   set(LIBSUFFIX "")
endif()
set(INSTALL_LIB_DIR     lib${LIBSUFFIX} CACHE PATH "Installation directory for libraries")
mark_as_advanced(INSTALL_LIB_DIR)

if (VISTLE_USE_CUDA)
   vistle_find_package(CUDA)
   if ("${CUDA_VERSION}" VERSION_LESS 7.0)
      set (CUDA_HOST_COMPILER ${PROJECT_SOURCE_DIR}/scripts/cuda-host-compiler CACHE STRING "CUDA nvcc host compiler" FORCE)
   else()
      set(CUDA_PROPAGATE_HOST_FLAGS OFF)
      set(CUDA_NVCC_FLAGS "--std=c++11 ${CUDA_NVCC_FLAGS}")
   endif()
   if (APPLE)
      set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} \"-DBOOST_NOINLINE=__attribute__ ((noinline))\"")
   endif (APPLE)
endif()

macro(vistle_add_library targetname)
    add_library(${targetname} ${ARGN})
    set_target_properties(${targetname} PROPERTIES FOLDER "Libraries")
    if (CRAY)
       set_property(TARGET ${targetname} APPEND_STRING PROPERTY LINK_FLAGS -Wl,-soname=lib${targetname}.so)
    endif()
    install(TARGETS ${targetname}
       LIBRARY DESTINATION ${INSTALL_LIB_DIR}
       ARCHIVE DESTINATION ${INSTALL_LIB_DIR})
endmacro(vistle_add_library)

if (APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

add_subdirectory(3rdparty)
add_subdirectory(vistle)

if(${CMAKE_VERSION} VERSION_GREATER "2.8.2")
   include(FeatureSummary)
   feature_summary(WHAT ALL)
endif()                                                                                                                                                            
