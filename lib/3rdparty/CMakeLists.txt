#project(vistle_3rdparty)

#add_subdirectory(covconfig)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

set(CMAKE_C_VISIBILITY_PRESET default)
set(CMAKE_CXX_VISIBILITY_PRESET default)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996 /wd4267 /wd4800 /wd4244")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996 /wd4267 /wd4800 /wd4244")
endif()

if(NOT VISTLE_USE_MPI)
    vistle_add_library(
        vistle_mpi
        EXPORT
        ${VISTLE_LIB_TYPE}
        mpi-serial/mpi.c
        mpi-serial/send.c
        mpi-serial/recv.c
        mpi-serial/collective.c
        mpi-serial/req.c
        mpi-serial/list.c
        mpi-serial/handles.c
        mpi-serial/comm.c
        mpi-serial/error.c
        mpi-serial/ic_merge.c
        mpi-serial/group.c
        mpi-serial/time.c
        mpi-serial/pack.c
        mpi-serial/type.c
        mpi-serial/type_const.c
        mpi-serial/copy.c
        mpi-serial/op.c
        mpi-serial/cart.c
        mpi-serial/getcount.c
        mpi-serial/probe.c
        mpi-serial/message.c
        mpi-serial/win.c
        mpi-serial/io.c
        mpi-serial/info.c)
    target_compile_definitions(vistle_mpi PRIVATE FORTRAN_SAME)
    target_compile_definitions(vistle_mpi PRIVATE MPI_NO_FORTRAN)
    target_compile_definitions(vistle_mpi PRIVATE _XOPEN_SOURCE=600) # for gethostname on Linux despite -std=c99
    target_include_directories(vistle_mpi PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/mpi-serial> $<INSTALL_INTERFACE:include>)
    if(WIN32)
        target_link_libraries(vistle_mpi PRIVATE ws2_32)
    endif()
    add_library(MPI::MPI_C ALIAS vistle_mpi)
    add_library(MPI::MPI_CXX ALIAS vistle_mpi)
endif()

if(BOOST_MPI_DIR)
    include(boost-mpi.cmake)
endif()

set(BUILD_SHARED_LIBS
    ${VISTLE_BUILD_SHARED}
    CACHE INTERNAL "")

if(NOT BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
endif()

if(VISTLE_PROFILE_NVTX)
    add_subdirectory(profile_nvtx/c)
    vistle_export_library(nvtx3-c)
    vistle_export_library(nvtx3-cpp)
endif()

if(USE_SZ3)
    add_subdirectory(sz3)
endif()

#
# BigWhoop
#
include_directories(${BIGWHOOP_INCLUDE_DIRS})
add_subdirectory(BigWhoop)

# enable POSIX features required by BigWhoop
target_compile_definitions(bwclib PRIVATE _POSIX_C_SOURCE=200809L)
set_target_properties(bwclib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
# we must call this command (taken from BigWhoop/src/library/CMakeLists.txt)
# to generate the public header file for BigWhoop
execute_process(
    COMMAND python3 public_header.py ${PYTHON_ARGUMENT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BigWhoop
    RESULT_VARIABLE result
    ERROR_VARIABLE error_output)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    vistle_find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(bwclib PRIVATE OpenMP::OpenMP_CXX)
    endif()
endif()

if(VISTLE_INTERNAL_ZFP)
    set(BUILD_TESTING
        OFF
        CACHE INTERNAL "")
    set(BUILD_UTILITIES
        OFF
        CACHE INTERNAL "")
    set(ZFP_LIBRARY_PREFIX
        "vistle_"
        CACHE STRING "Prefix to prepend to the output library name")
    mark_as_advanced(ZFP_LIBRARY_PREFIX)
    set(ZFP_WITH_OPENMP ${VISTLE_USE_OPENMP})
    set(CMAKE_INSTALL_INCLUDEDIR include/vistle/3rdparty)
    add_subdirectory(zfp)
endif()

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
if(NOT VISTLE_GUI_ONLY)
    if(VISTLE_INTERNAL_VISKORES)
        #
        # Viskores
        #
        #set(Viskores_NO_ASSERT ON)
        set(Viskores_NO_DEPRECATED_VIRTUAL ON)
        set(Viskores_ENABLE_MPI OFF)
        #self.define_from_variant("Viskores_ENABLE_OPENMP", "openmp"),
        set(Viskores_ENABLE_DEVELOPER_FLAGS OFF)
        set(Viskores_ENABLE_RENDERING OFF)
        set(Viskores_ENABLE_TESTING OFF)
        set(Viskores_ENABLE_TESTING_LIBRARY OFF)
        set(Viskores_INSTALL_EXAMPLES OFF)
        set(Viskores_ENABLE_BENCHMARKS OFF)
        set(Viskores_ENABLE_DOCUMENTATION OFF)
        set(Viskores_ENABLE_EXAMPLES OFF)
        set(Viskores_ENABLE_CPACK OFF)
        set(Viskores_USE_64BIT_IDS ${VISTLE_64BIT_INDICES})
        #set(Viskores_USE_DEFAULT_TYPES_FOR_ASCENT", "ascent_types"),
        set(Viskores_USE_DOUBLE_PRECISION ${VISTLE_DOUBLE_PRECISION})

        set(Viskores_ENABLE_OPENMP ${VISTLE_USE_OPENMP})
        set(Viskores_ENABLE_CUDA ${VISTLE_CUDA_ENABLED})
        set(Viskores_ENABLE_KOKKOS ${VISTLE_USE_KOKKOS})
        if(NOT ${Viskores_ENABLE_KOKKOS}
           AND NOT ${Viskores_ENABLE_CUDA}
           AND NOT ${Viskores_ENABLE_OPENMP})
            set(Viskores_ENABLE_TBB ${TBB_FOUND})
        endif()

        #set(Viskores_INSTALL_LIB_DIR "${_vtk_build_LIBRARY_DESTINATION}")
        #set(Viskores_INSTALL_INCLUDE_DIR "${_vtk_build_HEADERS_DESTINATION}/vtkviskores/viskores")
        #if (DEFINED _vtk_build_LIBRARY_NAME_SUFFIX)
        #  set(Viskores_CUSTOM_LIBRARY_SUFFIX "-${_vtk_build_LIBRARY_NAME_SUFFIX}")
        #endif ()
        #set(Viskores_EXECUTABLE_OUTPUT_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        #set(Viskores_LIBRARY_OUTPUT_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
        set(Viskores_BUILD_CMAKE_BASE_DIR "${CMAKE_BINARY_DIR}")
        #set(Viskores_INSTALL_CONFIG_DIR "${_vtk_build_CMAKE_DESTINATION}/viskores")
        #set(Viskores_INSTALL_CMAKE_MODULE_DIR "${Viskores_INSTALL_CONFIG_DIR}/cmake")
        # Currently Viskores only installs its readme and license.
        #set(Viskores_INSTALL_SHARE_DIR "${_vtk_build_LICENSE_DESTINATION}/vtkviskores")

        #set(Viskores_INSTALL_ONLY_LIBRARIES OFF)

        #set(Viskores_USE_DEFAULT_SYMBOL_VISIBILITY OFF)
        set(Viskores_SKIP_LIBRARY_VERSIONS ON)

        set(Viskores_DEFAULT_TYPES_HEADER "../../../ViskoresDefaultTypesVistle.h.in")

        set(Viskores_MODULE_ENABLE_viskores_io "NO")
        set(Viskores_MODULE_ENABLE_viskores_source "NO")

        set(Viskores_GROUP_ENABLE_Filters "DONT_WANT")
        set(Viskores_GROUP_ENABLE_FiltersCommon "DONT_WANT")
        set(Viskores_MODULE_ENABLE_viskores_filter_contour "YES")
        set(Viskores_MODULE_ENABLE_viskores_filter_field_conversion "YES")

        set(Viskores_MODULE_ENABLE_viskores_filter_clean_grid "YES")
        set(Viskores_MODULE_ENABLE_viskores_filter_connected_components "NO")
        set(Viskores_MODULE_ENABLE_viskores_filter_density_esimate "NO")
        set(Viskores_MODULE_ENABLE_viskores_filter_entity_extraction "YES")
        set(Viskores_MODULE_ENABLE_viskores_filter_field_transform "YES")
        set(Viskores_MODULE_ENABLE_viskores_filter_flow "NO")
        set(Viskores_MODULE_ENABLE_viskores_filter_geometry_refinement "YES")
        set(Viskores_MODULE_ENABLE_viskores_filter_image_processing "NO")
        #set(Viskores_MODULE_ENABLE_viskores_filter_mesh_info "NO")
        set(Viskores_MODULE_ENABLE_viskores_filter_multiblock "NO")
        set(Viskores_MODULE_ENABLE_viskores_filter_resampling "NO")
        set(Viskores_MODULE_ENABLE_viskores_filter_scalar_topology "NO")
        set(Viskores_MODULE_ENABLE_viskores_filter_testing "NO")
        set(Viskores_MODULE_ENABLE_viskores_filter_uncertainty "NO")
        #set(Viskores_MODULE_ENABLE_viskores_filter_vector_analysis "NO")
        set(Viskores_MODULE_ENABLE_viskores_filter_zfp "NO")

        add_subdirectory(viskores)
        add_library(viskores::cont ALIAS viskores_cont)
        add_library(viskores::filter ALIAS viskores_filter)
        add_library(viskores::worklet ALIAS viskores_worklet)
        #add_library(viskores_io ALIAS viskores::io)
        #add_library(viskores_rendering ALIAS viskores::rendering)
        #add_library(viskores_source ALIAS viskores::source)
        #add_library(viskores_worklet ALIAS viskores::worklet)

    endif()

    if(VISTLE_USE_MPI)
        set(ICET_SUBMODULE TRUE)
        #
        # IceT
        #
        #set(BUILD_SHARED_LIBS ${VISTLE_BUILD_SHARED} CACHE INTERNAL "")
        set(ICET_BUILD_TESTING
            OFF
            CACHE INTERNAL "")
        set(ICET_USE_OPENGL
            OFF
            CACHE INTERNAL "")
        set(ICET_USE_OPENGL3
            OFF
            CACHE INTERNAL "")
        set(ICET_INSTALL_NO_RUNTIME
            TRUE
            CACHE INTERNAL "")
        set(ICET_INSTALL_NO_DEVELOPMENT
            TRUE
            CACHE INTERNAL "")
        set(LIBRARY_OUTPUT_PATH
            ${PROJECT_BINARY_DIR}/lib
            CACHE PATH "Output directory for building all libraries.")
        set(EXECUTABLE_OUTPUT_PATH
            ${PROJECT_BINARY_DIR}/lib
            CACHE PATH "Output directory for building all binaries.")
        set(ICET_LIBRARY_DIR
            ${PROJECT_BINARY_DIR}/lib
            CACHE PATH "Output directory for building all libraries.")
        set(ICET_EXECUTABLE_DIR
            ${PROJECT_BINARY_DIR}/bin
            CACHE PATH "Output directory for building all executable.")
        set(ICET_BINARY_DIR
            ${PROJECT_BINARY_DIR}
            CACHE PATH "Value overridden by Vistle")
        set(ICET_INSTALL_LIB_DIR
            ${INSTALL_LIB_DIR}
            CACHE INTERNAL "")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            add_compile_options(-Wno-variadic-macros -Wno-pedantic)
        endif()
        add_subdirectory(IceT)
    endif()

    #
    # pybind11
    #
    set(PYBIND11_FINDPYTHON
        TRUE
        CACHE BOOL "")
    add_subdirectory(pybind11)
endif()

add_library(vistle_yas INTERFACE)
target_include_directories(vistle_yas SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/yas/include>)
vistle_export_library(vistle_yas)

if(VISTLE_INSTALL_3RDPARTY)
    #yas
    install(
        DIRECTORY yas/include/yas
        DESTINATION include/vistle/3rdparty
        COMPONENT Devel)

    #eigen3
    install(
        DIRECTORY eigen3
        DESTINATION include/vistle/3rdparty
        COMPONENT Devel)
endif()

# vistle_find_package(Eigen3 REQUIRED)
add_library(vistle_eigen INTERFACE)
set(EIGEN_DEFINITIONS "-DEIGEN_DEFAULT_DENSE_INDEX_TYPE=int" "-DEIGEN_DONT_PARALLELIZE") # also disable OpenMP for eigen
target_compile_definitions(vistle_eigen INTERFACE ${EIGEN_DEFINITIONS})
#set(EIGEN3_INCLUDE_PATH "${PROJECT_SOURCE_DIR}/lib/3rdparty")
target_include_directories(vistle_eigen SYSTEM BEFORE INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>)
# Export as title case Eigen
set_target_properties(vistle_eigen PROPERTIES EXPORT_NAME VistleEigen)
vistle_export_library(vistle_eigen)

#install (TARGETS eigen EXPORT Eigen3Targets)

# add_definitions(-DEIGEN_DEFAULT_DENSE_INDEX_TYPE=short)

include(cmrc/CMakeRC.cmake)
vistle_export_library(cmrc-base)
