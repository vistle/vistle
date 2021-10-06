#project(vistle)

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

    macro(add_module targetname description)

        file(APPEND ${MODULE_DESCRIPTION_FILE} ${targetname} " " ${description}\n)
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
        target_link_libraries(${targetname} ${Boost_LIBRARIES} ${MPI_C_LIBRARIES} vistle_module ${BOOST_MPI})
    endmacro(add_module)
endif()

set(VISTLE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=512")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    if(VISTLE_PEDANTIC_ERRORS)
        add_compile_options("-pedantic-errors")
    endif()
    if(NOT APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
    else()
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error")
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
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
set(THRUST_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/3rdparty/thrust")

#vistle_find_package(Eigen3 REQUIRED)
set(EIGEN3_INCLUDE_PATH "../3rdparty")
set(DIY_INCLUDE_PATH "${PROJECT_SOURCE_DIR}/3rdparty/diy/include")

if (NOT VISTLE_GUI_ONLY)
    #Boost.MPI is compiled with Vistle - for easy switching of MPI versions
    include_directories(BEFORE SYSTEM "${PROJECT_SOURCE_DIR}/3rdparty/${BOOST_MPI_DIR}/include")
endif()
if(Boost_FOUND)
    include_directories(AFTER SYSTEM ${Boost_INCLUDE_DIRS})
endif()

# header-only python bindings
include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/3rdparty/pybind11/include")

# replacement for Boost.Serialization
#include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/3rdparty/yas/include")

# lossy floating point compression
#vistle_find_package(ZFP)
set(ZFP_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/3rdparty/zfp/include")
set(ZFP_LIBRARIES zfp)
if(OPENMP_FOUND)
    set(ZFP_LIBRARIES ${ZFP_LIBRARIES} ${OpenMP_CXX_LIBRARIES})
endif()
set(ZFP_FOUND TRUE)
add_definitions(-DHAVE_ZFP)
include_directories(SYSTEM "${ZFP_INCLUDE_DIRS}")

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
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
vistle_find_package(Threads REQUIRED)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(COVISEDESTDIR ${PROJECT_BINARY_DIR})

add_subdirectory(util)
add_subdirectory(core)
add_subdirectory(net)
add_subdirectory(userinterface)
add_subdirectory(gui)

if(VISTLE_GUI_ONLY)
    return()
endif()

add_subdirectory(rhr)
add_subdirectory(bin)
add_subdirectory(control)
add_subdirectory(hub)
add_subdirectory(insitu)
add_subdirectory(vtk)

add_subdirectory(module)
add_subdirectory(renderer)
if(NOT VISTLE_MODULES_SHARED)
    add_subdirectory(module_descriptions)
endif()
#message("Vistle modules: ${ALL_MODULES}")
# keep after module and renderer, as ALL_MODULES will be populated within those two directories

vistle_find_package(COVER) # required for spinning Qt event loop in single-process config
add_subdirectory(manager)
add_subdirectory(cover)

add_subdirectory(test)
