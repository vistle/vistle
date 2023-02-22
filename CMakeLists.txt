if(WIN32)
    cmake_minimum_required(VERSION 3.15.2 FATAL_ERROR)
else(WIN32)
    cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED 99)
set(CMAKE_C_EXTENSIONS OFF)

project(Vistle C CXX)

if(APPLE AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    set(CMAKE_PREFIX_PATH /opt/homebrew ${CMAKE_PREFIX_PATH})
endif()

message("CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
if("${CMAKE_SYSTEM_NAME}" STREQUAL "CrayLinuxEnvironment")
    message("compiling for Cray")
    set(CRAY ON)
    #set(Boost_DEBUG ON)
    set(Boost_NO_BOOST_CMAKE OFF)
    set(CRAYPE_LINK_TYPE "$ENV{CRAYPE_LINK_TYPE}")
    if(NOT "${CRAYPE_LINK_TYPE}" STREQUAL "dynamic")
        message("dynamic linking required: set CRAYPE_LINK_TYPE to \"dynamic\"")
        return()
    endif()

    # use faster multi-threaded MPI
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -craympich-mt")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -craympich-mt")

    # work around broken FindCUDA
    macro(find_host_program)
        set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
        if(CMAKE_HOST_WIN32)
            set(WIN32 1)
            set(UNIX)
        elseif(CMAKE_HOST_APPLE)
            set(APPLE 1)
            set(UNIX)
        endif()
        find_program(${ARGN})
        set(WIN32)
        set(APPLE)
        set(UNIX 1)
        set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    endmacro()
endif()

add_custom_target(vistle_doc)
add_custom_target(vistle_doc_skip)

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

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    option(VISTLE_PEDANTIC_ERRORS "Be pedantic about C++ standard and emit errors" OFF) # for diy
else()
    option(VISTLE_PEDANTIC_ERRORS "Be pedantic about C++ standard and emit errors" ON)
endif()

option(VISTLE_SANITIZE_UNDEFINED "Build with -fsanitize=undefined" OFF)
option(VISTLE_SANITIZE_ADDRESS "Build with -fsanitize=address" OFF)
option(VISTLE_SANITIZE_THREAD "Build with -fsanitize=thread" OFF)
option(VISTLE_TIME_BUILD "Gather build times (Clang/ISPC)" OFF)

option(VISTLE_USE_CUDA "Build with CUDA support" OFF)

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
    set(CMAKE_DEFAULT_BUILD_TYPE "RelWithDebInfo")
elseif(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE
        "RelWithDebInfo"
        CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
    message("Setting CMAKE_BUILD_TYPE to ${CMAKE_BUILD_TYPE}")
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")

    else()
        add_definitions(-DEIGEN_INITIALIZE_MATRICES_BY_NAN)
    endif()
endif()

#add_definitions(-DEIGEN_DEFAULT_DENSE_INDEX_TYPE=short)
add_definitions(-DEIGEN_DEFAULT_DENSE_INDEX_TYPE=int)

# use this instead of FIND_PACKAGE to prefer Package in $PACKAGE_HOME and $EXTERNLIBS/package

macro(vistle_find_package package)
    set(SAVED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    set(SAVED_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})

    set(pack "${package}")
    if(pack STREQUAL "PythonLibs")
        set(pack "Python")
    endif()
    if(pack STREQUAL "PythonInterp")
        set(pack "Python")
    endif()
    if(pack MATCHES "^Qt5")
        set(pack "Qt5")
        if(APPLE)
            set(CMAKE_PREFIX_PATH /usr/local/opt/qt5 ${CMAKE_PREFIX_PATH})
            if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
                set(CMAKE_PREFIX_PATH /opt/homebrew/opt/qt5 ${CMAKE_PREFIX_PATH})
            endif()
        endif()
    endif()
    if(pack STREQUAL "COVISE" OR pack STREQUAL "COVER")
        if(NOT "$ENV{COVISEDIR}" STREQUAL "")
            set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/cmake" ${CMAKE_MODULE_PATH})
            set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/share/cmake" ${CMAKE_MODULE_PATH})
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
    #message("looking for package ${ARGV}")
    #message("CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
    find_package(${ARGV})

    set(CMAKE_PREFIX_PATH ${SAVED_CMAKE_PREFIX_PATH})
    set(CMAKE_MODULE_PATH ${SAVED_CMAKE_MODULE_PATH})
endmacro(vistle_find_package PACKAGE)

if(WIN32)
    #set(BOOST_ROOT "$ENV{EXTERNLIBS}/boost")
    #STRING(REGEX REPLACE "\\\\" "/" BOOST_ROOT ${BOOST_ROOT})
    #set(MPI_HOME "$ENV{EXTERNLIBS}/OpenMPI")
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/windows;${CMAKE_MODULE_PATH}")
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_WARNINGS} /EHsc")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MSVC_WARNINGS} /EHsc")
endif(WIN32)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/;${CMAKE_MODULE_PATH}")

include(ConfigureLinker)

if(NOT VISTLE_GUI_ONLY)
    find_package(OpenMP)
    if(OPENMP_FOUND)
        find_library(LIBOMP omp)
    endif()

    if(CMAKE_VERSION VERSION_LESS 3.12)
        message(WARNING "CMake 3.12 or newer is required for compiling Python support")
    else()
        vistle_find_package(Python COMPONENTS Interpreter Development REQUIRED)
        if(Python_FOUND)
            message("Found Python version ${Python_VERSION}")
        endif()
    endif()

    #vistle_find_package(IceT REQUIRED)
    set(ICET_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/3rdparty/IceT/src/include;${CMAKE_BINARY_DIR}/lib/3rdparty/IceT/src/include)
    set(ICET_CORE_LIBS "IceTCore")
    set(ICET_MPI_LIBS "IceTMPI")
    set(ICET_FOUND TRUE)

    vistle_find_package(TBB)
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
    #set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize-address-use-after-scope")
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
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
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

#set (Boost_DEBUG ON)
if(WIN32)
    #otherwise Boost_INCLUDE_DIRS is not set and headers are not found
    set(Boost_NO_BOOST_CMAKE ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
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
    1.59
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
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

    if(NOT ${BUILD_SHARED_LIBS})
        # for linking the libraries into plugins
        #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
        set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")

endif()

if(NOT CRAY)
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

        execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
        if(GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
            set(CUDA_HOST_COMPILER
                ${CMAKE_SOURCE_DIR}/scripts/cuda-host-compiler
                CACHE STRING "CUDA nvcc host compiler" FORCE)
        endif()

    endif()

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    #elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif()

if(NOT CRAY AND NOT VISTLE_GUI_ONLY)
    set(MPI_SKIP_MPICXX TRUE)
    #set(MPI_FIND_COMPONENTS "C CXX")
    add_definitions(-DOMPI_SKIP_MPICXX) # OpenMPI
    add_definitions(-DMPI_NO_CPPBIND) # HPE MPT
    vistle_find_package(MPI REQUIRED COMPONENTS C CXX)

    include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})
    set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS}")
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")

    # for CMake config file version of boost this needs to be changed to version numbers such as 1.60.99 instead of 106099
    if(NOT VISTLE_INTERNAL_BOOST_MPI)
        message("Using external Boost.MPI for Boost ${Boost_VERSION}")
    elseif(Boost_VERSION VERSION_GREATER 106999)
        set(BOOST_MPI_DIR "boost-mpi")
    elseif(Boost_VERSION VERSION_GREATER 106099)
        set(BOOST_MPI_DIR "boost-mpi-1.69")
    elseif(Boost_VERSION VERSION_GREATER 1.73.99)
        set(BOOST_MPI_DIR "boost-mpi")
    elseif(Boost_VERSION VERSION_GREATER 1.69.99)
        set(BOOST_MPI_DIR "boost-mpi-1.69")
    else()
        set(BOOST_MPI_DIR "boost-mpi-1.60")
    endif()
    if(BOOST_MPI_DIR)
        message("Using Boost.MPI from ${BOOST_MPI_DIR} for Boost ${Boost_VERSION}")
        set(BOOST_MPI vistle_boost_mpi)
    else()
        set(BOOST_MPI Boost::mpi)
    endif()
endif()

add_definitions(-DBOOST_LIB_DIAGNOSTIC=1)

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if(LIB64)
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX "")
endif()
if(NOT WIN32 AND NOT APPLE)
    set(INSTALL_LIB_DIR
        lib${LIBSUFFIX}
        CACHE PATH "Installation directory for libraries")
    mark_as_advanced(INSTALL_LIB_DIR)
endif()

if(VISTLE_USE_CUDA AND NOT VISTLE_GUI_ONLY)
    vistle_find_package(CUDA)
    if("${CUDA_VERSION}" VERSION_LESS 7.0)
        set(CUDA_HOST_COMPILER
            ${PROJECT_SOURCE_DIR}/scripts/cuda-host-compiler
            CACHE STRING "CUDA nvcc host compiler" FORCE)
    else()
        set(CUDA_PROPAGATE_HOST_FLAGS OFF)
        set(CUDA_NVCC_FLAGS "--std=c++11 ${CUDA_NVCC_FLAGS}")
    endif()
    if(APPLE)
        set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} \"-DBOOST_NOINLINE=__attribute__ ((noinline))\"")
    endif(APPLE)
endif()

include(GNUInstallDirs)

macro(vistle_install_library targetname)
    if(${CMAKE_VERSION} VERSION_LESS "3.12.4")
        install(
            TARGETS ${targetname}
            EXPORT ${targetname}Targets
            INCLUDES
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${targetname}_Runtime
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${targetname}_Runtime
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${targetname}_Development)
    else()
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
    endif()
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
    #message("configuring file ${config_in_file} to ${buildPackageLocation}/${targetname}Config.cmake")

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
    #	configure_package_config_file(${config_in_file}
    #	"${CMAKE_CURRENT_BINARY_DIR}/${targetname}Config.cmake"
    #	INSTALL_DESTINATION "${installPackageLocation}/${targetname}Config.cmake")
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
    elseif() #change the vistle_libname to vistle::libname in exported target
        string(REPLACE "vistle_" "" target_suffix ${targetname})
        set_target_properties(${targetname} PROPERTIES EXPORT_NAME ${target_suffix})
        add_library(vistle::${target_suffix} ALIAS ${targetname})
    endif()

    if(CRAY)
        set_property(
            TARGET ${targetname}
            APPEND_STRING
            PROPERTY LINK_FLAGS -Wl,-soname=lib${targetname}.so)
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

function(getPublicBoostComponents PublicBoostComponents)
    set(addNext FALSE)
    foreach(Comp IN LISTS ARGN)
        if(addNext)
            string(REGEX REPLACE "::.*" "" TARGET_NAME ${Comp})
            if(${TARGET_NAME} STREQUAL "Boost")
                string(REGEX REPLACE ".*::" "" COMPONENT_NAME ${Comp})
                list(APPEND LocalPublicBoostComponents ${COMPONENT_NAME})
            endif()
            set(addNext FALSE)
        else()
            if(${Comp} MATCHES "PUBLIC|INTERFACE")
                set(addNext TRUE)
            endif()
        endif()
    endforeach()
    set(${PublicBoostComponents}
        ${LocalPublicBoostComponents}
        PARENT_SCOPE)
endfunction()

#additionally adds public and interface dependencies to Config.cmake file
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

    getpublicboostcomponents(public_boost_componets ${ARGN})
    list(LENGTH public_boost_componets num_PUBLIC_Boost_componets)
    if(num_PUBLIC_Boost_componets GREATER 0)
        set(file_content ${file_content} "find_dependency( Boost " ${BOOST_MINIMUM_REQUIRED_VERSION} " REQUIRED COMPONENTS \n")
        foreach(item IN LISTS public_boost_componets)
            set(file_content ${file_content} "\t" ${item} "\n")
        endforeach()
        set(file_content ${file_content} ")\n")
    endif()
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

#install VistleConfig.cmake
configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/VistleConfig.cmake ${PROJECT_BINARY_DIR}/lib/cmake/vistle/VistleConfig.cmake COPYONLY)
install(FILES ${CMAKE_CURRENT_LIST_DIR}/cmake/VistleConfig.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vistle)

if(APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

if(POLICY CMP0043)
    # configuration (RelWithDebInfo, Debug, ...) dependent COMPILE_DEFINITIONS are not used
    # - default to new behavior
    cmake_policy(SET CMP0043 NEW)
endif()

if(WIN32)
    option(VISTLE_PYTHON_DEBUG "for Debug builds, use expect Python built in Debug mode" OFF)
    if(VISTLE_PYTHON_DEBUG)
        add_compile_options("$<$<CONFIG:DEBUG>:-DPy_DEBUG>")
    endif()
endif()

enable_testing()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# configure process model
if(CRAY)
    option(VISTLE_MULTI_PROCESS "use multiple processes communicating via shared memory" OFF)
else()
    option(VISTLE_MULTI_PROCESS "use multiple processes communicating via shared memory" ON)
endif()
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

# configure data-type sizes
option(VISTLE_64BIT_INDICES "use 64-bit integers for array indices" OFF)
if(VISTLE_64BIT_INDICES)
    add_definitions(-DVISTLE_INDEX_64BIT)
endif()

option(VISTLE_DOUBLE_PRECISION "use double instead of float for coordinates and data arrays" OFF)
if(VISTLE_DOUBLE_PRECISION)
    add_definitions(-DVISTLE_SCALAR_DOUBLE)
endif()

macro(USE_OPENMP)
    if(OPENMP_FOUND)
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

    set(VISTLE_DOC_SKIP FALSE)
    file(REMOVE ${MODULE_DESCRIPTION_FILE})
    install(FILES ${MODULE_DESCRIPTION_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/vistle/)

    macro(add_module targetname description)

        if(NOT VISTLE_MODULE_CATEGORY)
            set(VISTLE_MODULE_CATEGORY "Unspecified")
        endif()
        file(APPEND ${MODULE_DESCRIPTION_FILE} "${targetname} ${VISTLE_MODULE_CATEGORY} ${description}\n")
        set(ALL_MODULES
            ${ALL_MODULES} ${targetname}
            CACHE INTERNAL "")
        if(VISTLE_MULTI_PROCESS)
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
                #add_library(${targetname} OBJECT ${ARGN})
                add_library(${targetname} STATIC ${ARGN})
            endif()
        endif()
        target_compile_definitions(${targetname} PRIVATE VISTLE_MODULE_NAME="${targetname}")
        #target_compile_definitions(${targetname} PRIVATE MODULE_DESCRIPTION="${description}")

        set_target_properties(${targetname} PROPERTIES FOLDER "Modules")
        target_link_libraries(${targetname} ${Boost_LIBRARIES} ${MPI_C_LIBRARIES} vistle_module ${BOOST_MPI} vistle_config)
        if(USE_LIBOMP AND LIBOMP)
            #message("Linking ${targetname} against libomp: ${LIBOMP}")
            target_link_libraries(${targetname} ${LIBOMP})
        endif()

        set_target_properties(${targetname} PROPERTIES FOLDER "Modules")
        add_module_doc_target(${targetname} ${VISTLE_DOC_SKIP})
    endmacro(add_module)

    include(cmake/Documentation.cmake)
endif()

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(VISTLE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    if(VISTLE_COLOR_DIAGNOSTICS)
        add_compile_options("-fcolor-diagnostics")
    endif()
    if(VISTLE_TIME_BUILD)
        add_compile_options("-ftime-trace")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=512")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
    if(VISTLE_PEDANTIC_ERRORS)
        add_compile_options("-pedantic-errors")
    endif()
    if(NOT APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
    else()
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error")
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    if(VISTLE_COLOR_DIAGNOSTICS)
        add_compile_options("-fdiagnostics-color=always")
    endif()
    if(VISTLE_PEDANTIC_ERRORS)
        add_compile_options("-pedantic-errors")
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

#set(CMAKE_BUILD_TYPE Debug)

option(VISTLE_SHM_DEBUG "Shared memory debugging" OFF)
if(VISTLE_SHM_DEBUG)
    add_definitions(-DSHMDEBUG)
endif()

set(ENABLE_INSTALLER TRUE)
#vistle_find_package(Thrust REQUIRED)
set(THRUST_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/3rdparty/thrust")

#new submodule for block distribution
set(DIY_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/3rdparty/diy/include")

#vistle_find_package(Eigen3 REQUIRED)
set(EIGEN3_INCLUDE_PATH "${PROJECT_SOURCE_DIR}/lib/3rdparty")

if(NOT VISTLE_GUI_ONLY)
    #Boost.MPI is compiled with Vistle - for easy switching of MPI versions
    include_directories(BEFORE SYSTEM "${PROJECT_SOURCE_DIR}/lib/3rdparty/${BOOST_MPI_DIR}/include")
endif()
if(Boost_FOUND)
    include_directories(AFTER SYSTEM ${Boost_INCLUDE_DIRS})
endif()

# header-only python bindings
include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/lib/3rdparty/pybind11/include")
add_definitions(-DPYBIND11_DETAILED_ERROR_MESSAGES)

# replacement for Boost.Serialization
#include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/lib/3rdparty/yas/include")

# lossy floating point compression
#vistle_find_package(ZFP)
set(ZFP_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/lib/3rdparty/zfp/include")
set(ZFP_LIBRARIES zfp)
if(OPENMP_FOUND)
    set(ZFP_LIBRARIES ${ZFP_LIBRARIES} ${OpenMP_CXX_LIBRARIES})
endif()
set(ZFP_FOUND TRUE)
add_definitions(-DHAVE_ZFP)
include_directories(SYSTEM "${ZFP_INCLUDE_DIRS}")

vistle_find_package(ZSTD)
if(CMAKE_VERSION VERSION_LESS 3.18)
    message("Not building with SZ3 support: CMake older than 3.18")
    set(USE_SZ3 FALSE)
elseif(NOT ZSTD_FOUND)
    message("Not building with SZ3 support: no ZSTD")
    set(USE_SZ3 FALSE)
else()
    set(SZ3_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/lib/3rdparty/sz3/include;${PROJECT_BINARY_DIR}/lib/3rdparty/sz3/include")
    set(SZ3_FOUND TRUE)
    add_definitions(-DHAVE_SZ3)
    include_directories(SYSTEM "${SZ3_INCLUDE_DIRS}")
    set(USE_SZ3 TRUE)
endif()

if(NOT VISTLE_GUI_ONLY)
    vistle_find_package(
        OpenSceneGraph
        3.2.0
        COMPONENTS
        osgViewer
        osgGA
        osgDB
        osgUtil
        osgText
        osgParticle
        osgSim
        osgFX)
    set(OpenGL_GL_PREFERENCE LEGACY)
    vistle_find_package(OpenGL)

    vistle_find_package(COVER)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
vistle_find_package(Threads REQUIRED)

vistle_find_package(
    VTK
    5.0
    COMPONENTS
    vtkCommonDataModel
    vtkIOXML
    vtkIOLegacy
    #vtkCommonCore
    #vtkFiltersCore
    #vtkImagingCore
    #vtkCommonExecutionModel
    NO_MODULE
    QUIET)

# do it here, so that we see the warnings just once
vistle_find_package(Xdmf QUIET)
if(WIN32 AND Xdmf_FOUND)
    vistle_find_package(Iconv)
    if(NOT Iconv_FOUND)
        unset(Xdmf_FOUND)
    endif()
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
