include(FindPackageHandleStandardArgs)
vistle_find_package(embree 3.0)
find_package_handle_standard_args(EMBREE DEFAULT_MSG EMBREE_LIBRARY EMBREE_INCLUDE_DIRS)

vistle_find_package(ZLIB)

if(NOT ISPC_EXECUTABLE)
    find_program(ISPC_EXECUTABLE ispc DOC "Path to the ISPC executable.")
endif()

if(NOT EMBREE_FOUND)
    message("DisCOVERay: Embree not found")
    return()
endif()

if(NOT ISPC_EXECUTABLE)
    message("DisCOVERay: ISPC not found")
    return()
endif()

set(USE_TBB FALSE)
if(OPENMP_FOUND)
    use_openmp()
elseif(TBB_FOUND)
    add_definitions(-DUSE_TBB)
    set(USE_TBB TRUE)
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    add_compile_options("-Wno-c99-extensions")
    add_compile_options("-Wno-gnu-variable-sized-type-not-at-end")
endif()

# BEGIN: ispc
##############################################################
# ISA configuration
##############################################################

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(ISPC_TARGETS "neon-i32x8")
    #list(APPEND ISPC_TARGETS "neon-i32x4")
    set(__XEON__ 1)
else()
    if(EMBREE_MAX_ISA STREQUAL "NONE")
        set(MAX_ISA 0)
    endif()

    if(EMBREE_MAX_ISA STREQUAL "NONE")
        set(XEON_ISA
            "AVX512"
            CACHE STRING "Selects ISA to support.")
    else()
        set(XEON_ISA
            "RANGE"
            CACHE STRING "Selects ISA to support.")
    endif()
    set_property(
        CACHE XEON_ISA
        PROPERTY STRINGS
                 RANGE
                 SSE2
                 SSE3
                 SSSE3
                 SSE4.1
                 SSE4.2
                 AVX
                 AVX-I
                 AVX2
                 AVX512)

    if(EMBREE_MAX_ISA STREQUAL "SSE2")
        set(MAX_ISA 1)
    endif()

    if(EMBREE_MAX_ISA STREQUAL "SSE3")
        set(MAX_ISA 2)
    endif()

    if(EMBREE_MAX_ISA STREQUAL "SSSE3")
        set(MAX_ISA 3)
    endif()

    if(EMBREE_MAX_ISA STREQUAL "SSE4.1")
        set(MAX_ISA 4)
    endif()

    if(EMBREE_MAX_ISA STREQUAL "SSE4.2")
        set(MAX_ISA 5)
    endif()

    if(EMBREE_MAX_ISA STREQUAL "AVX")
        set(MAX_ISA 6)
    endif()

    if(EMBREE_MAX_ISA STREQUAL "AVX-I")
        set(MAX_ISA 7)
    endif()

    if(EMBREE_MAX_ISA STREQUAL "AVX2")
        set(MAX_ISA 8)
    endif()

    if(EMBREE_MAX_ISA STREQUAL "AVX512")
        set(MAX_ISA 9)
    endif()

    if(MAX_ISA GREATER 0 OR EMBREE_ISA_SSE2)
        set(ISPC_TARGETS "sse2")
    endif()

    if(MAX_ISA GREATER 3 OR EMBREE_ISA_SSE42)
        list(APPEND ISPC_TARGETS "sse4")
    endif()

    if(MAX_ISA GREATER 5 OR EMBREE_ISA_AVX)
        list(APPEND ISPC_TARGETS "avx")
    endif()

    if(MAX_ISA GREATER 7 OR EMBREE_ISA_AVX2)
        list(APPEND ISPC_TARGETS "avx2")
    endif()

    if(MAX_ISA GREATER 8 OR EMBREE_ISA_AVX512SKX)
        list(APPEND ISPC_TARGETS "avx512skx-i32x16")
    endif()

    set(__XEON__ 1)
endif()
set(ENABLE_ISPC_SUPPORT ON)
include(ispc)
include_directories_ispc(${EMBREE_INCLUDE_DIRS})
# END: ispc

set(RAY_HEADERS common.h rayrenderobject.h)
set(RAY_SOURCES ray.cpp rayrenderobject.cpp)
set(ISPC_HEADERS render.isph renderobjectdata.isph spheres_isect.isph spheres.isph tubes.isph)
set(ISPC_SOURCES render.ispc renderobjectdata.ispc spheres.ispc tubes.ispc)

set(ALL_MODULES
    ${ALL_MODULES} DisCOVERay
    CACHE INTERNAL "")
add_definitions(-DVISTLE_MODULE_NAME="DisCOVERay")
if((CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo") OR (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    set(ISPC_COMPILE_FLAGS "-g")
endif()
if(VISTLE_DOUBLE_PRECISION)
    set(ISPC_COMPILE_FLAGS "${ISPC_COMPILE_FLAGS}" -DVISTLE_SCALAR_DOUBLE)
endif()

if(VISTLE_MULTI_PROCESS)
    add_ispc_executable(DisCOVERay ${RAY_HEADERS} ${RAY_SOURCES} ${ISPC_SOURCES})
    set_target_properties(DisCOVERay PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/libexec/module)
    install(TARGETS DisCOVERay RUNTIME DESTINATION libexec/module)
else()
    if(VISTLE_MODULES_SHARED)
        set(type MODULE)
    else()
        set(type STATIC)
    endif()
    add_ispc_library(DisCOVERay ${type} ${RAY_HEADERS} ${RAY_SOURCES} ${ISPC_SOURCES})
    set_target_properties(DisCOVERay PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/module LIBRARY_OUTPUT_DIRECTORY
                                                                                                          ${PROJECT_BINARY_DIR}/lib/module)
    if(VISTLE_MODULES_SHARED)
        install(TARGETS DisCOVERay LIBRARY DESTINATION lib/module)
    endif()
endif()
add_custom_target(ispc SOURCES ${ISPC_SOURCES} ${ISPC_HEADERS})

include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})
include_directories(SYSTEM ${EMBREE_INCLUDE_DIRS})
set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")

target_link_libraries(
    DisCOVERay
    ${BOOST_MPI}
    vistle_module
    vistle_renderer
    vistle_rhr
    ${ZLIB_LIBRARIES}
    ${EMBREE_LIBRARY}
    MPI::MPI_C)

if(USE_LIBOMP AND LIBOMP)
    target_link_libraries(DisCOVERay ${LIBOMP})
endif()

if(USE_TBB)
    include_directories(SYSTEM ${TBB_INCLUDE_DIRS})
    target_link_libraries(DisCOVERay ${TBB_LIBRARIES})
endif()

set(description "Embree CPU ray casting remote renderer")
file(APPEND ${MODULE_DESCRIPTION_FILE} DisCOVERay " " ${description}\n)
#target_compile_definitions(DisCOVERay PRIVATE MODULE_DESCRIPTION="${description}")
