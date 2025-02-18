cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED 99)
set(CMAKE_C_EXTENSIONS OFF)

project(
    Vistle
    DESCRIPTION "Parallel distributed scientific visualization in VR"
    HOMEPAGE_URL https://vistle.io
    LANGUAGES C CXX)

message("CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")

if(POLICY CMP0042)
    # default to finding shlibs relative to @rpath on MacOS
    cmake_policy(SET CMP0042 NEW)
endif()

# Automatically link Qt executables to qtmain target on Windows
cmake_policy(SET CMP0020 NEW)

if(POLICY CMP0074)
    # make find_include/find_library search in <PackageName>_ROOT prefix
    cmake_policy(SET CMP0074 NEW)
endif()
if(POLICY CMP0144)
    # make find_include/find_library search in uppercased <PackageName>_ROOT prefix
    cmake_policy(SET CMP0144 NEW)
endif()

# vim's YouCompleteMe can make use of this
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    option(VISTLE_GUI_ONLY "Only graphical workflow editor" ON)
    set(Boost_USE_STATIC_RUNTIME TRUE)
    option(VISTLE_BUILD_SHARED "Build all libraries as shared libraries instead of static" OFF)
else()
    option(VISTLE_GUI_ONLY "Only graphical workflow editor" OFF)
    option(VISTLE_BUILD_SHARED "Build all libraries as shared libraries instead of static" ON)
endif()

set(VISTLE_CPU_ARCH
    ""
    CACHE STRING "CPU architecture to optimize for (native, corei7-avx, core-avx2, atom, ...)")

option(VISTLE_PEDANTIC_ERRORS "Be pedantic about C++ standard and emit errors" OFF) # for diy, also included in vtk-m

option(VISTLE_SANITIZE_UNDEFINED "Build with -fsanitize=undefined" OFF)
option(VISTLE_SANITIZE_ADDRESS "Build with -fsanitize=address" OFF)
option(VISTLE_SANITIZE_THREAD "Build with -fsanitize=thread" OFF)
option(VISTLE_TIME_BUILD "Gather build times (Clang/ISPC)" OFF)

option(VISTLE_PROFILE_NVTX "Annotate for Nsight profiler" OFF)
option(VISTLE_PROFILE_CHROME "Annotate for showing timings in Chrome browser" OFF)

option(VISTLE_USE_CUDA "Build with CUDA support" OFF)
option(VISTLE_USE_KOKKOS "Build with Kokkos support" OFF)
option(VISTLE_USE_OPENMP "Build with OpenMP support" OFF)

option(VISTLE_USE_QT5 "Build GUI with Qt5 instead of Qt6" OFF)
option(VISTLE_USE_MPI "Build with MPI instead of dummy MPI library" ON)

option(VISTLE_COLOR_DIAGNOSTICS "Produce ANSI-colored build output (GNU/Clang)" ON)

if(NOT "$ENV{VBUILD}" STREQUAL "")
    include("${PROJECT_SOURCE_DIR}/build/$ENV{VBUILD}.cmake")
endif()

option(VISTLE_INSTALL_3RDPARTY "Add the 3rdParty directories to the install directory" OFF)
option(VISTLE_INSTALL_HEADER "Install the Vistle headers in CMAKE_INSTALL_PREFIX/include/path to lib" ON)

if(VISTLE_BUILD_SHARED)
    set(VISTLE_LIB_TYPE SHARED)
else()
    set(VISTLE_LIB_TYPE STATIC)
    add_definitions(-DVISTLE_STATIC)
    set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
endif()

option(VISTLE_INTERNAL_BOOST_MPI "Use internal copy of Boost.MPI" ON)

# allow for optimizations in Boost.MPI
if(VISTLE_INTERNAL_BOOST_MPI)
    if(NOT WIN32)
        add_definitions(-DBOOST_MPI_HOMOGENEOUS)
    endif()
endif()

enable_testing()

if(CMAKE_CONFIGURATION_TYPES)
    if(${CMAKE_GENERATOR} STREQUAL "Ninja Multi-Config")
        set(CMAKE_DEFAULT_BUILD_TYPE "RelWithDebInfo")
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE
        "RelWithDebInfo"
        CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
    message("Setting CMAKE_BUILD_TYPE to ${CMAKE_BUILD_TYPE}")
endif()

# use this instead of FIND_PACKAGE to prefer Package in $PACKAGE_HOME and $EXTERNLIBS/package
macro(vistle_find_package package)
    set(SAVED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    set(SAVED_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})

    set(pack "${package}")
    set(hbpack "")

    if(pack STREQUAL "COVISE" OR pack STREQUAL "COVER")
        if(NOT "$ENV{COVISEDIR}" STREQUAL "")
            set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/cmake" ${CMAKE_MODULE_PATH})
            set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/share/cmake" ${CMAKE_MODULE_PATH})
        endif()
    endif()

    if("$ENV{SPACK_ENV}" STREQUAL "")
        if(pack STREQUAL "PythonLibs")
            set(pack "Python")
        endif()

        if(pack STREQUAL "PythonInterp")
            set(pack "Python")
        endif()

        if(pack MATCHES "^Qt5")
            set(pack "Qt5")
            set(hbpack "qt@5")
        endif()

        if(pack MATCHES "^Qt6")
            set(pack "Qt6")
            set(hbpack "qt@6")
        endif()

        if(APPLE)
            if(pack MATCHES "^OpenMP")
                set(hbpack "libomp")
            endif()

            if(pack STREQUAL "Python")
                set(hbpack "python@3")
            endif()

            if(pack STREQUAL "OpenSceneGraph")
                set(hbpack "open-scene-graph")
            endif()

            if(pack STREQUAL "TIFF")
                set(hbpack "libtiff")
            endif()

            if(pack STREQUAL "JPEGTURBO")
                set(hbpack "jpeg-turbo")
            endif()

            if(pack MATCHES "^Qt5")
                set(pack "Qt5")
                set(hbpack "qt@5")
            endif()

            if(pack MATCHES "^Qt6")
                set(pack "Qt6")
                set(hbpack "qt@6")
            endif()

            if(hbpack STREQUAL "")
                set(hbpack "${pack}")
            endif()

            set(CMAKE_PREFIX_PATH /usr/local/opt/${hbpack} ${CMAKE_PREFIX_PATH})

            if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
                set(CMAKE_PREFIX_PATH /opt/homebrew/opt/${hbpack} ${CMAKE_PREFIX_PATH})
            endif()
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

        # message("looking for package ${ARGV}")
        # message("CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
    endif()

    find_package(${ARGV})

    if(APPLE)
        if(pack MATCHES "^FLEX$")
            if(FLEX_EXECUTABLE AND NOT FLEX_INCLUDE_DIR)
                set(CMAKE_PREFIX_PATH /Library/Developer/CommandLineTools/usr ${CMAKE_PREFIX_PATH})
                find_package(${ARGV})
            endif()
        endif()
    endif()

    set(CMAKE_PREFIX_PATH ${SAVED_CMAKE_PREFIX_PATH})
    set(CMAKE_MODULE_PATH ${SAVED_CMAKE_MODULE_PATH})
endmacro(vistle_find_package PACKAGE)

if(WIN32)
    # set(BOOST_ROOT "$ENV{EXTERNLIBS}/boost")
    # STRING(REGEX REPLACE "\\\\" "/" BOOST_ROOT ${BOOST_ROOT})
    # set(MPI_HOME "$ENV{EXTERNLIBS}/OpenMPI")
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/windows;${CMAKE_MODULE_PATH}")

    # add_definitions(-DOMPI_IMPORTS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)

    add_definitions(-DBOOST_ALL_NO_LIB)

    # add_definitions(-DBOOST_ALL_DYN_LINK)
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_WARNINGS} /EHsc")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MSVC_WARNINGS} /EHsc")
endif(WIN32)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/;${CMAKE_MODULE_PATH}")

# figure out version
set(VISTLE_VERSION_TAG "unknown")
set(VISTLE_VERSION_HASH "unknown")
if(EXISTS "${PROJECT_SOURCE_DIR}/.vistle_version.txt")
    # this is generated by bin/_print_version.sh and should match the git output below
    file(READ "${PROJECT_SOURCE_DIR}/.vistle_version.txt" FULL_VERSION)
    string(REGEX REPLACE "\n$" "" FULL_VERSION "${FULL_VERSION}")
    string(REGEX REPLACE "-.*$" "" VISTLE_VERSION_TAG "${FULL_VERSION}")
    string(REGEX REPLACE "^.*-" "" VISTLE_VERSION_HASH "${FULL_VERSION}")
    message("VISTLE_VERSION: ${VISTLE_VERSION_TAG}-${VISTLE_VERSION_HASH}")
else()
    # make sure this matches bin/_print_version.sh
    vistle_find_package(Git)

    if(GIT_FOUND)
        # for hash
        execute_process(
            COMMAND ${GIT_EXECUTABLE} log -n1 --format=%h/%ci
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_LOG_OUT
            ERROR_VARIABLE GIT_LOG_ERROR
            RESULT_VARIABLE GIT_LOG_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        # for latest tag contained by hash
        execute_process(
            COMMAND ${GIT_EXECUTABLE} tag --sort=v:refname --merged
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_TAG_OUT
            ERROR_VARIABLE GIT_TAG_ERROR
            RESULT_VARIABLE GIT_TAG_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        # working copy clean/dirty?
        execute_process(
            COMMAND ${GIT_EXECUTABLE} status --untracked-files=no --porcelain
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_STATUS_OUT
            ERROR_VARIABLE GIT_STATUS_ERROR
            RESULT_VARIABLE GIT_STATUS_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        if(${GIT_LOG_RESULT} EQUAL 0
           AND ${GIT_TAG_RESULT} EQUAL 0
           AND ${GIT_STATUS_RESULT} EQUAL 0)
            string(REGEX REPLACE "^([a-z0-9]*)/(....)-(..)-(..).*" "\\1" VISTLE_VERSION_HASH "${GIT_LOG_OUT}")
            if(NOT "" STREQUAL "${GIT_STATUS_OUT}")
                # append + to hash, if dirty
                set(VISTLE_VERSION_HASH "${VISTLE_VERSION_HASH}+")
            endif()

            set(VISTLE_VERSION_TAG "${GIT_TAG_OUT}")
            # ensure that every tag is surrounded by \n
            string(PREPEND VISTLE_VERSION_TAG "\n")
            string(APPEND VISTLE_VERSION_TAG "\n")

            # throw out everything that does not start with v20
            string(REGEX REPLACE "\n([^v]|.[^2]|..[^0])[^\n]*" "\n" VISTLE_VERSION_TAG "${VISTLE_VERSION_TAG}")
            # remove empty lines
            string(REGEX REPLACE "\n+" "\n" VISTLE_VERSION_TAG "${VISTLE_VERSION_TAG}")
            # remove final \n
            string(REGEX REPLACE "\n$" "" VISTLE_VERSION_TAG "${VISTLE_VERSION_TAG}")
            # remove everything up to and including last \n
            string(REGEX REPLACE "^.*\n" "" VISTLE_VERSION_TAG "${VISTLE_VERSION_TAG}")
            # chop off leading v
            string(REGEX REPLACE "^v" "" VISTLE_VERSION_TAG "${VISTLE_VERSION_TAG}")
            #message("VISTLE_VERSION_TAG: ${VISTLE_VERSION_TAG}")
        else()
            if(NOT ${GIT_LOG_RESULT} EQUAL 0)
                message("Command \"${GIT_EXECUTABLE} log -n1 --format=%H/%ci ${dir}\" failed with output:\n${GIT_LOG_ERROR}")
            endif()
            if(NOT ${GIT_TAG_RESULT} EQUAL 0)
                message("Command \"${GIT_EXECUTABLE} tag --merged ${dir}\" failed with output:\n${GIT_TAG_ERROR}")
            endif()
        endif()
    endif()
    message("VISTLE_VERSION: ${VISTLE_VERSION_TAG}-${VISTLE_VERSION_HASH}-git")
endif()

include(ConfigureLinker)

if(NOT VISTLE_GUI_ONLY)
    set(VISTLE_USE_SEPARABLE_COMPILATION TRUE)
    set(VISTLE_CUDA_ENABLED FALSE)
    if(VISTLE_USE_KOKKOS)
        vistle_find_package(Kokkos REQUIRED)
        if(Kokkos_ENABLE_HIP)
            enable_language(HIP)
            set(VISTLE_USE_SEPARABLE_COMPILATION FALSE)
        endif()
        if(Kokkos_ENABLE_CUDA)
            set(VISTLE_CUDA_ENABLED TRUE)
        endif()
        if(Kokkos_ENABLE_OPENMP)
            if(NOT VISTLE_USE_OPENMP)
                message("OpenMP support enabled, because required by Kokkos")
            endif()
            set(VISTLE_USE_OPENMP TRUE)
        endif()
    endif()

    if(VISTLE_USE_CUDA OR VISTLE_CUDA_ENABLED)
        include(CheckLanguage)
        check_language(CUDA)
        vistle_find_package(CUDAToolkit)

        if(CMAKE_CUDA_COMPILER AND CUDAToolkit_FOUND)
            enable_language(CUDA)

            if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
                set(CMAKE_CUDA_ARCHITECTURES 60) # be compatible with P6000/GP100
            endif()

            add_definitions(-DVISTLE_USE_CUDA)
            set(VISTLE_CUDA_ENABLED TRUE)
            message("Using CUDA")
        else()
            set(VISTLE_CUDA_ENABLED FALSE)
        endif()
    endif()

    if(VISTLE_USE_OPENMP)
        vistle_find_package(OpenMP 3.0 COMPONENTS CXX)

        if(OPENMP_FOUND)
            find_library(LIBOMP omp)
        endif()
    endif()

    if(NOT OPENMP_FOUND)
        # prefer a TBBConfig.cmake over FindTBB.cmake
        vistle_find_package(TBB CONFIG NO_MODULE)

        if(NOT ${TBB_FOUND})
            vistle_find_package(TBB)

            if(NOT (${CMAKE_GENERATOR} STREQUAL "Visual Studio 17 2022"))
                if(${TBB_FOUND} AND NOT TARGET TBB::tbb)
                    add_library(TBB::tbb SHARED IMPORTED)
                    set_target_properties(
                        TBB::tbb
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${TBB_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES "${TBB_LIBRARIES}"
                                   IMPORTED_LOCATION "${TBB_LIBRARY}")
                endif()
            endif()
        endif()
    endif()

    vistle_find_package(Python COMPONENTS Interpreter Development REQUIRED)
    if(Python_FOUND)
        message("Found Python version ${Python_VERSION}")
    endif()

    # vistle_find_package(IceT REQUIRED)
    set(ICET_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/3rdparty/IceT/src/include;${CMAKE_BINARY_DIR}/lib/3rdparty/IceT/src/include)
    set(ICET_CORE_LIBS "IceTCore")
    set(ICET_MPI_LIBS "IceTMPI")
    set(ICET_FOUND TRUE)

endif(NOT VISTLE_GUI_ONLY)

set(ASAN_COMPILE_FLAGS "")
set(ASAN_LINK_FLAGS "")

if(VISTLE_SANITIZE_UNDEFINED)
    set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=undefined")
    set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=undefined")
endif()

if(VISTLE_SANITIZE_ADDRESS)
    set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=address")
    set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=address")

    # set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize-address-use-after-scope")
    set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fno-common")
endif()

if(VISTLE_SANITIZE_THREAD)
    set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=thread")
    set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=thread")
endif()

if(VISTLE_SANITIZE_THREAD
   OR VISTLE_SANITIZE_ADDRESS
   OR VISTLE_SANITIZE_UNDEFINED)
    set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fno-omit-frame-pointer")
    set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -g3")
    set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fPIC")

    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_COMPILE_FLAGS}")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} ${ASAN_LINK_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${ASAN_LINK_FLAGS}")

if(WIN32)
    set(BOOST_OPT zlib)
    set(BOOST_LIBRARYDIR $ENV{EXTERNLIBS}/boost/lib)
else()
    set(BOOST_OPT)
endif()

if(NOT VISTLE_GUI_ONLY)
    set(BOOST_OPT ${BOOST_OPT} iostreams locale)
endif()

if(VISTLE_INTERNAL_BOOST_MPI)
    set(BOOST_REQ)
else()
    set(BOOST_REQ mpi)
endif()

# set (Boost_DEBUG ON)
if(WIN32)
    # otherwise, in older versions Boost_INCLUDE_DIRS is not set and headers are not found
    # set(Boost_NO_BOOST_CMAKE ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
    option(VISTLE_Boost_USE_STATIC_LIBS "use boost static libs" OFF)
    set(Boost_USE_STATIC_LIBS ${VISTLE_Boost_USE_STATIC_LIBS})
endif(WIN32)

if(APPLE)
    # force use of pthread_mutexattr_setpshared on macos:
    # we do not use condition variables, so the bug that not providing this
    # define is supposed to work around is not relevant to Vistle, but the
    # lack of mutexes working between processes is a real problem
    # cf. https://github.com/boostorg/interprocess/issues/53
    add_definitions(-DBOOST_INTERPROCESS_POSIX_PROCESS_SHARED)
endif()

vistle_find_package(
    Boost
    1.66
    REQUIRED
    COMPONENTS
    system
    serialization
    program_options
    date_time
    filesystem
    chrono
    thread
    regex
    timer
    ${BOOST_REQ}
    OPTIONAL_COMPONENTS
    ${BOOST_OPT})

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-command-line-argument")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")

    if(NOT ${BUILD_SHARED_LIBS})
        # for linking the libraries into plugins
        set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

    if(NOT ${BUILD_SHARED_LIBS})
        # for linking the libraries into plugins
        set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" AND CMAKE_CXX_COMPILER_VERSION MATCHES "^11")
        # silence "note: parameter passing for argument of type ‘vtkm::Vec<double, 2>’ when C++17 is enabled changed to match C++14 in GCC 10.1"
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")

endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wno-stdlibcxx-not-found")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

    if(DEFINED VISTLE_CPU_ARCH AND NOT VISTLE_CPU_ARCH STREQUAL "")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${VISTLE_CPU_ARCH}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${VISTLE_CPU_ARCH}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

    if(NOT APPLE)
        if(DEFINED VISTLE_CPU_ARCH AND NOT VISTLE_CPU_ARCH STREQUAL "")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${VISTLE_CPU_ARCH}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${VISTLE_CPU_ARCH}")
        endif()
    endif()
endif()

if(NOT VISTLE_GUI_ONLY)
    if(VISTLE_USE_MPI)
        set(MPI_SKIP_MPICXX TRUE)
        vistle_find_package(MPI REQUIRED COMPONENTS C CXX)
    endif()

    # for CMake config file version of boost this needs to be changed to version numbers such as 1.60.99 instead of 106099
    if(NOT VISTLE_INTERNAL_BOOST_MPI)
        message("Using external Boost.MPI for Boost ${Boost_VERSION}")
    elseif(Boost_VERSION VERSION_GREATER 106999)
        set(BOOST_MPI_DIR "boost-mpi")
    elseif(Boost_VERSION VERSION_GREATER 106099)
        set(BOOST_MPI_DIR "boost-mpi-1.69")
    elseif(Boost_VERSION VERSION_GREATER 1.73.99)
        set(BOOST_MPI_DIR "boost-mpi")
    else()
        set(BOOST_MPI_DIR "boost-mpi-1.69")
    endif()

    if(BOOST_MPI_DIR)
        message("Using Boost.MPI from ${BOOST_MPI_DIR} for Boost ${Boost_VERSION}")
        set(BOOST_MPI vistle_boost_mpi)
    else()
        set(BOOST_MPI Boost::mpi)
    endif()
endif()

add_definitions(-DBOOST_LIB_DIAGNOSTIC=1)

if(NOT WIN32 AND NOT APPLE)
    get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
    if(LIB64)
        set(LIBSUFFIX 64)
    else()
        set(LIBSUFFIX "")
    endif()

    set(INSTALL_LIB_DIR
        lib${LIBSUFFIX}
        CACHE PATH "Installation directory for libraries")
    mark_as_advanced(INSTALL_LIB_DIR)
endif()

include(GNUInstallDirs)

macro(vistle_install_library targetname)
    install(
        TARGETS ${targetname}
        EXPORT ${targetname}Targets
        INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${targetname}_Runtime
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT ${targetname}_Runtime
                NAMELINK_COMPONENT ${targetname}_Development
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${targetname}_Development)

    if("${vistle_install_include_directory}" STREQUAL "")
        string(REPLACE ${PROJECT_BINARY_DIR}/lib/vistle "" vistle_install_include_directory ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    set(header "${ARGN}")
    list(FILTER header INCLUDE REGEX "(\.(h|H|hpp|HPP)$)")

    if(VISTLE_INSTALL_HEADER)
        install(
            FILES ${header}
            DESTINATION include/vistle/${vistle_install_include_directory}
            COMPONENT Devel)
    endif()
endmacro(vistle_install_library)

macro(vistle_export_library targetname)
    vistle_install_library(${targetname} ${ARGN})

    set(config_in_file ${CMAKE_CURRENT_BINARY_DIR}/cmake/${targetname}Config.cmake.in)
    file(WRITE ${config_in_file} "include(\"\${CMAKE_CURRENT_LIST_DIR}/${targetname}Targets.cmake\")")

    # message("configuring file ${config_in_file} to ${buildPackageLocation}/${targetname}Config.cmake")
    configure_file(${config_in_file} "${buildPackageLocation}/${targetname}Config.cmake" COPYONLY)
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${targetname}ConfigVersion.cmake"
        VERSION "1.0.0"
        COMPATIBILITY ExactVersion)
    configure_file(${CMAKE_CURRENT_BINARY_DIR}/${targetname}ConfigVersion.cmake ${buildPackageLocation}/${targetname}ConfigVersion.cmake COPYONLY)
    export(
        EXPORT ${targetname}Targets
        FILE "${buildPackageLocation}/${targetname}Targets.cmake"
        NAMESPACE Vistle::)

    install(
        EXPORT ${targetname}Targets
        FILE ${targetname}Targets.cmake
        NAMESPACE Vistle::
        DESTINATION ${installPackageLocation}
        COMPONENT ${targetname}_Development)

    # configure_package_config_file(${config_in_file}
    # "${CMAKE_CURRENT_BINARY_DIR}/${targetname}Config.cmake"
    # INSTALL_DESTINATION "${installPackageLocation}/${targetname}Config.cmake")
    install(
        FILES "${buildPackageLocation}/${targetname}Config.cmake" "${buildPackageLocation}/${targetname}ConfigVersion.cmake"
        DESTINATION ${installPackageLocation}
        COMPONENT ${targetname}_Development)
endmacro(vistle_export_library)

macro(vistle_add_library targetname visibility)
    add_library(${targetname} ${ARGN})
    set_target_properties(${targetname} PROPERTIES FOLDER "Libraries")
    string(FIND ${targetname} "vistle_" vistle_prefix)

    if(NOT ${vistle_prefix} EQUAL 0)
        if(NOT "${targetname}" STREQUAL "VistlePluginUtil"
           AND NOT "${targetname}" STREQUAL "simV2runtime_par"
           AND NOT "${targetname}" STREQUAL "simV2runtime_ser")
            message(WARNING " missing prefix for " ${targetname}": vistle libraries must be prefixed with vistle_")
        endif()
    elseif() # change the vistle_libname to vistle::libname in exported target
        string(REPLACE "vistle_" "" target_suffix ${targetname})
        set_target_properties(${targetname} PROPERTIES EXPORT_NAME ${target_suffix})
        add_library(vistle::${target_suffix} ALIAS ${targetname})
    endif()

    target_include_directories(
        ${targetname}
        PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        PRIVATE $<INSTALL_INTERFACE:include>
        INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        INTERFACE $<INSTALL_INTERFACE:include>)

    if("${visibility}" STREQUAL "EXPORT")
        vistle_export_library(${targetname} ${ARGN})
    elseif(NOT "${visibility}" STREQUAL "INTERNAL")
        message(FATAL_ERROR "vistle_add_library: visibility argument wrong: ${visibility}")
    endif()
endmacro(vistle_add_library)

set(buildPackageLocation ${PROJECT_BINARY_DIR}/lib/cmake/vistle)
set(installPackageLocation ${CMAKE_INSTALL_LIBDIR}/cmake/vistle)

function(prepend_file filename)
    if(EXISTS ${filename})
        file(READ ${filename} old_content)
        file(WRITE ${filename} ${ARGN} ${old_content})
    endif()
endfunction()

function(getPublicComponents Name PublicComponents)
    set(addNext FALSE)

    foreach(Comp IN LISTS ARGN)
        if(addNext)
            string(REGEX REPLACE "::.*" "" TARGET_NAME ${Comp})

            if(${TARGET_NAME} STREQUAL "${Name}")
                string(REGEX REPLACE ".*::" "" COMPONENT_NAME ${Comp})
                list(APPEND LocalPublicComponents ${COMPONENT_NAME})
            endif()

            set(addNext FALSE)
        else()
            if(${Comp} MATCHES "PUBLIC|INTERFACE")
                set(addNext TRUE)
            endif()
        endif()
    endforeach()

    set(${PublicComponents}
        ${LocalPublicComponents}
        PARENT_SCOPE)
endfunction()

# additionally adds public and interface dependencies to Config.cmake file
macro(vistle_target_link_libraries targetname)
    target_link_libraries(${targetname} ${ARGN})
    set(file_content "include(CMakeFindDependencyMacro)\n")
    set(file_content ${file_content} "set(oldCMAKE_PREFIX_PATH \${CMAKE_PREFIX_PATH})\n")
    set(file_content ${file_content} "set(CMAKE_PREFIX_PATH \${CMAKE_CURRENT_LIST_DIR} \${CMAKE_PREFIX_PATH})\n")
    set(add_next_arg false)
    set(hasDependencies false)

    foreach(arg IN ITEMS ${ARGN})
        if(add_next_arg)
            if(${arg} MATCHES "vistle_.*")
                set(file_content ${file_content} "if(NOT " ${arg} "_DIR)\n")
                set(file_content ${file_content} "  set(" ${arg} "_DIR @" ${arg} "_DIR@)\n")
                set(file_content ${file_content} "endif()\n")
                set(file_content ${file_content} "find_dependency(" ${arg} ")\n")
                set(add_next_arg false)
            endif()
        elseif(${arg} MATCHES "PUBLIC|INTERFACE" OR (VISTLE_MODULES_SHARED AND ${arg} MATCHES "PRIVATE"))
            set(add_next_arg true)
            set(hasDependencies true)
        endif()
    endforeach()

    set(DependencyList Boost vtkm) # Name when linking the package
    set(DependencyListNames Boost VTKm) # Name when finding the package
    set(CurrentDependencyList 0)

    foreach(Dependency IN ITEMS ${DependencyList})
        getpubliccomponents(${Dependency} public_componets ${ARGN})
        list(LENGTH public_componets num_Public_componets)
        list(GET DependencyListNames ${CurrentDependencyList} CurrentDependencyName)
        math(EXPR CurrentDependencyList "${CurrentDependencyList}+1")

        if(num_Public_componets GREATER 0)
            set(file_content ${file_content} "find_dependency( ${CurrentDependencyName} " ${BOOST_MINIMUM_REQUIRED_VERSION} " REQUIRED COMPONENTS \n")

            foreach(item IN LISTS public_componets)
                set(file_content ${file_content} "\t" ${item} "\n")
            endforeach()

            set(file_content ${file_content} ")\n")
        endif()
    endforeach()

    set(file_content ${file_content} "set(CMAKE_PREFIX_PATH \${oldCMAKE_PREFIX_PATH})\n")
    set(file_content ${file_content} "find_dependency(MPI REQUIRED)\n")

    if(hasDependencies)
        prepend_file(${buildPackageLocation}/${targetname}Config.cmake ${file_content})
    endif()
endmacro()

macro(vistle_install_docs)
    string(REPLACE ${PROJECT_BINARY_DIR} "" directory ${CMAKE_CURRENT_BINARY_DIR})
    install(
        FILES "${ARGN}"
        DESTINATION doc/${directory}
        COMPONENT Devel)
endmacro(vistle_install_docs)

vistle_install_docs(README.md)

macro(vistle_install_headers directory)
    install(FILES ${ARGN} DESTINATION include/vistle/${directory})
endmacro(vistle_install_headers)

# install VistleConfig.cmake
configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/VistleConfig.cmake ${PROJECT_BINARY_DIR}/lib/cmake/vistle/VistleConfig.cmake COPYONLY)
install(FILES ${CMAKE_CURRENT_LIST_DIR}/cmake/VistleConfig.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vistle)

if(POLICY CMP0043)
    # configuration (RelWithDebInfo, Debug, ...) dependent COMPILE_DEFINITIONS are not used
    # - default to new behavior
    cmake_policy(SET CMP0043 NEW)
endif()

if(WIN32)
    option(VISTLE_PYTHON_DEBUG "for Debug builds, use expect Python built in Debug mode" ON)

    if(VISTLE_PYTHON_DEBUG)
        add_compile_options("$<$<CONFIG:DEBUG>:-DPy_DEBUG>")
    endif()
endif()

enable_testing()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# configure process model
option(VISTLE_MULTI_PROCESS "use multiple processes communicating via shared memory" ON)

option(VISTLE_MODULES_SHARED "use shared libraries for modules" ON)
option(VISTLE_USE_SHARED_MEMORY "use shared memory even within a process" OFF)

if(NOT VISTLE_MULTI_PROCESS)
    add_definitions(-DMODULE_THREAD)

    if(NOT VISTLE_MODULES_SHARED)
        add_definitions(-DMODULE_STATIC)
    endif()

    if(NOT VISTLE_USE_SHARED_MEMORY)
        add_definitions(-DNO_SHMEM)
    endif()
endif()

macro(USE_OPENMP)
    if(VISTLE_USE_OPENMP AND OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OpenMP_C_FLAGS}")

        if(${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")
            set(USE_LIBOMP TRUE)

            if(LIBOMP)
                set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LIBOMP}")
                set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LIBOMP}")
            endif()
        endif()
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
    endif()
endmacro(USE_OPENMP)

macro(vistle_add_executable targetname)
    add_executable(${targetname} ${ARGN})
    set_target_properties(${targetname} PROPERTIES FOLDER "Executables")
    install(
        TARGETS ${targetname}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION bin)
endmacro(vistle_add_executable)

set(ALL_MODULES
    ""
    CACHE INTERNAL "")
set(MODULE_DESCRIPTION_FILE ${PROJECT_BINARY_DIR}/share/vistle/moduledescriptions.txt)

if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    file(REMOVE ${MODULE_DESCRIPTION_FILE})
    install(FILES ${MODULE_DESCRIPTION_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/vistle/)

    macro(add_module2 targetname executable description)
        if(NOT VISTLE_MODULE_CATEGORY)
            set(VISTLE_MODULE_CATEGORY "Unspecified")
        endif()

        file(APPEND ${MODULE_DESCRIPTION_FILE} "${targetname} ${VISTLE_MODULE_CATEGORY} ${description}\n")
        set(ALL_MODULES
            ${ALL_MODULES} ${targetname}
            CACHE INTERNAL "")

        if(${executable})
            add_executable(${targetname} ${ARGN})
            set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/libexec/module)
            install(TARGETS ${targetname} RUNTIME DESTINATION libexec/module)
        else()
            if(VISTLE_MODULES_SHARED)
                if(Boost_VERSION VERSION_GREATER 1.69.99)

                elseif(Boost_VERSION VERSION_LESS 106400)
                    message(FATAL_ERROR "At least Boost 1.64 is required for Boost.DLL")
                endif()

                add_library(${targetname} MODULE ${ARGN})
                set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/module)
                set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/module)
                install(
                    TARGETS ${targetname}
                    RUNTIME DESTINATION lib/module
                    LIBRARY DESTINATION lib/module)
            else()
                # add_library(${targetname} OBJECT ${ARGN})
                add_library(${targetname} STATIC ${ARGN})
            endif()
        endif()

        target_compile_definitions(${targetname} PRIVATE VISTLE_MODULE_NAME="${targetname}")

        # target_compile_definitions(${targetname} PRIVATE MODULE_DESCRIPTION="${description}")
        set_target_properties(${targetname} PROPERTIES FOLDER "Modules")
        target_link_libraries(${targetname} ${Boost_LIBRARIES} MPI::MPI_CXX vistle_module ${BOOST_MPI} vistle_config)

        if(USE_LIBOMP AND LIBOMP)
            # message("Linking ${targetname} against libomp: ${LIBOMP}")
            target_link_libraries(${targetname} ${LIBOMP})
        endif()
    endmacro(add_module2)

    macro(add_module targetname description)
        if(VISTLE_MULTI_PROCESS)
            add_module2(${targetname} TRUE ${description} ${ARGN})
        else()
            add_module2(${targetname} FALSE ${description} ${ARGN})
        endif()
    endmacro(add_module)

    macro(add_vtkm_module targetname description)
        set(SOURCES "${ARGN}")
        list(FIND SOURCES "DEVICE_SOURCES" IDX)
        list(REMOVE_ITEM SOURCES "DEVICE_SOURCES")
        add_module(${targetname} ${description} ${SOURCES})
        if(IDX GREATER_EQUAL 0)
            list(SUBLIST SOURCES ${IDX} -1 DEVSOURCES)
            vtkm_add_target_information(${targetname} MODIFY_CUDA_FLAGS DEVICE_SOURCES ${DEVSOURCES})
        endif()
        target_link_libraries(${targetname} vistle_vtkm vtkm::cont vtkm::filter)
        if(VISTLE_USE_CUDA AND VISTLE_USE_SEPARABLE_COMPILATION)
            set_target_properties(${targetname} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
        endif()
    endmacro(add_vtkm_module)
endif()

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(VISTLE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    if(VISTLE_COLOR_DIAGNOSTICS)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
    endif()

    if(VISTLE_TIME_BUILD)
        add_compile_options("-ftime-trace")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=512")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

    if(VISTLE_PEDANTIC_ERRORS)
        add_compile_options($<$<COMPILE_LANGUAGE:C>:-pedantic-errors>)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-pedantic-errors>)
    endif()

    if(NOT APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
    else()
        # emits warning with Apple Clang 15
        # set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error")
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    if(VISTLE_COLOR_DIAGNOSTICS)
        add_compile_options($<$<COMPILE_LANGUAGE:C>:-fdiagnostics-color=always>)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fdiagnostics-color=always>)
    endif()

    if(VISTLE_PEDANTIC_ERRORS)
        add_compile_options($<$<COMPILE_LANGUAGE:C>:-pedantic-errors>)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-pedantic-errors>)
    endif()

    if(NOT APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")

endif()

if(WIN32)
    set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -Wall")
else(WIN32)
    set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -Wall -Wno-unknown-pragmas")
endif(WIN32)

# set(CMAKE_BUILD_TYPE Debug)

option(VISTLE_SHM_DEBUG "Shared memory debugging" OFF)

if(VISTLE_SHM_DEBUG)
    add_definitions(-DSHMDEBUG)
endif()

set(ENABLE_INSTALLER TRUE)

# vistle_find_package(Thrust REQUIRED)
set(THRUST_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/3rdparty/thrust")

# new submodule for block distribution
set(DIY_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/3rdparty/diy/include")

if(NOT VISTLE_GUI_ONLY)
    # Boost.MPI is compiled with Vistle - for easy switching of MPI versions
    include_directories(BEFORE SYSTEM "${PROJECT_SOURCE_DIR}/lib/3rdparty/${BOOST_MPI_DIR}/include")
endif()

if(Boost_FOUND)
    include_directories(AFTER SYSTEM ${Boost_INCLUDE_DIRS})
endif()

# header-only python bindings
include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/lib/3rdparty/pybind11/include")
add_definitions(-DPYBIND11_DETAILED_ERROR_MESSAGES)

# replacement for Boost.Serialization
# include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/lib/3rdparty/yas/include")

option(VISTLE_INTERNAL_ZFP "Use own copy of zfp lossy floating point compressor" ON)
if(VISTLE_INTERNAL_ZFP)
    # lossy floating point compression
    set(ZFP_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/lib/3rdparty/zfp/include")

    set(ZFP_LIBRARIES zfp)
    if(OPENMP_FOUND)
        set(ZFP_LIBRARIES ${ZFP_LIBRARIES} ${OpenMP_CXX_LIBRARIES})
    endif()

    set(ZFP_FOUND TRUE)
    #add_definitions(-DHAVE_ZFP)
    #include_directories(SYSTEM "${ZFP_INCLUDE_DIRS}")
else()
    vistle_find_package(ZFP REQUIRED)
endif()

vistle_find_package(PkgConfig)
vistle_find_package(ZSTD)

set(SZ3_FOUND FALSE)
set(USE_SZ3 FALSE)
if(CMAKE_VERSION VERSION_LESS 3.18)
    message("Not building with SZ3 support: CMake older than 3.18")
elseif(NOT ZSTD_FOUND)
    message("Not building with SZ3 support: no ZSTD")
elseif(NOT PkgConfig_FOUND)
    message("Not building with SZ3 support: no PkgConfig")
else()
    set(SZ3_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/lib/3rdparty/sz3/include;${PROJECT_BINARY_DIR}/lib/3rdparty/sz3/include")
    set(SZ3_FOUND TRUE)
    set(USE_SZ3 TRUE)
endif()

set(OpenGL_GL_PREFERENCE LEGACY)
vistle_find_package(OpenGL)
if(APPLE)
    target_compile_definitions(OpenGL::GL INTERFACE -DGL_SILENCE_DEPRECATION)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
vistle_find_package(Threads REQUIRED)

# do it here, so that we see the warnings just once
vistle_find_package(Xdmf QUIET)

if(WIN32 AND Xdmf_FOUND)
    vistle_find_package(Iconv)

    if(NOT Iconv_FOUND)
        unset(Xdmf_FOUND)
    endif()
endif()

vistle_find_package(
    VTK
    5.0
    COMPONENTS
    vtkCommonDataModel
    vtkIOXML
    vtkIOLegacy
    # vtkCommonCore
    # vtkFiltersCore
    # vtkImagingCore
    # vtkCommonExecutionModel
    NO_MODULE
    QUIET)

option(VISTLE_64BIT_INDICES "use 64-bit integers for array indices" OFF)
option(VISTLE_DOUBLE_PRECISION "use double instead of float for coordinates and data arrays" OFF)

option(VISTLE_INTERNAL_VTKM "Use internal copy of VTK-m" ON)

if(VTK_FOUND AND VTK_HAS_VTKm)
    message("Using VTK-m provided by VTK ${VTK_VERSION}")

    if(VISTLE_INTERNAL_VTKM)
        message(WARNING "Provided VTK already contains VTK-m, cannot build internal copy of VTK-m. Disabling VISTLE_INTERNAL_VTKM...")
        set(VISTLE_INTERNAL_VTKM OFF)
    endif()
endif()

if(NOT VISTLE_INTERNAL_VTKM)
    # search without version argument, as it is not possible to search for all compatible versions (2.0 & 2.1) in one go
    vistle_find_package(VTKm REQUIRED)

    if(NOT ("${VTKm_VERSION_MAJOR}" EQUAL 2))
        message(FATAL_ERROR "Vtk-m version must be >= 2.0 and < 3.0 (but is ${VTKm_VERSION}).")
    endif()

    # make sure vtk-m and vistle data types are compatible (if not, overwrite user flags, accordingly)
    add_subdirectory(module/test/IsoSurfaceVtkm/compatibility)
endif()

# configure data-type sizes
if(VISTLE_64BIT_INDICES)
    add_definitions(-DVISTLE_INDEX_64BIT)
endif()

if(VISTLE_DOUBLE_PRECISION)
    add_compile_definitions(VISTLE_SCALAR_DOUBLE)
endif()

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(COVISEDESTDIR ${PROJECT_BINARY_DIR})

add_subdirectory(lib)
add_subdirectory(module)
add_subdirectory(app) # has to come after module subdirectory for static module registration
add_subdirectory(test)
add_subdirectory(bin)

include(FeatureSummary)
feature_summary(WHAT ALL)
