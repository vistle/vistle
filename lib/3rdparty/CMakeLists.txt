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
    target_include_directories(vistle_mpi PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/mpi-serial> $<INSTALL_INTERFACE:include>)

    add_library(MPI::MPI_C ALIAS vistle_mpi)
endif()

if(BOOST_MPI_DIR)
    include(boost-mpi.cmake)
endif()

if(CRAY)
    set(BUILD_SHARED_LIBS
        OFF
        CACHE INTERNAL "")
else()
    set(BUILD_SHARED_LIBS
        ${VISTLE_BUILD_SHARED}
        CACHE INTERNAL "")
endif()

if(NOT BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
endif()

if(USE_SZ3)
    add_subdirectory(sz3)
endif()

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
set(CMAKE_INSTALL_INCLUDEDIR include/vistle/3rdparty)
add_subdirectory(zfp)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
if(NOT VISTLE_GUI_ONLY)
    if(VISTLE_INTERNAL_VTKM)
        #
        # VTK-m
        #
        #set(VTKm_NO_ASSERT ON)
        set(VTKm_NO_DEPRECATED_VIRTUAL ON)
        set(VTKm_ENABLE_MPI OFF)
        #self.define_from_variant("VTKm_ENABLE_OPENMP", "openmp"),
        set(VTKm_ENABLE_DEVELOPER_FLAGS OFF)
        set(VTKm_ENABLE_RENDERING OFF)
        set(VTKm_ENABLE_TESTING OFF)
        set(VTKm_ENABLE_TESTING_LIBRARY OFF)
        set(VTKm_INSTALL_EXAMPLES OFF)
        set(VTKm_ENABLE_BENCHMARKS OFF)
        set(VTKm_ENABLE_DOCUMENTATION OFF)
        set(VTKm_ENABLE_EXAMPLES OFF)
        set(VTKm_ENABLE_CPACK OFF)
        set(VTKm_USE_64BIT_IDS ${VISTLE_64BIT_INDICES})
        #set(VTKm_USE_DEFAULT_TYPES_FOR_ASCENT", "ascent_types"),
        set(VTKm_USE_DOUBLE_PRECISION ${VISTLE_DOUBLE_PRECISION})

        set(VTKm_ENABLE_TBB ${TBB_FOUND})
        set(VTKm_ENABLE_CUDA ${VISTLE_USE_CUDA})
        set(VTKm_ENABLE_KOKKOS ${VISTLE_ENABLE_KOKKOS})

        #set(VTKm_INSTALL_LIB_DIR "${_vtk_build_LIBRARY_DESTINATION}")
        #set(VTKm_INSTALL_INCLUDE_DIR "${_vtk_build_HEADERS_DESTINATION}/vtkvtkm/vtk-m")
        #if (DEFINED _vtk_build_LIBRARY_NAME_SUFFIX)
        #  set(VTKm_CUSTOM_LIBRARY_SUFFIX "-${_vtk_build_LIBRARY_NAME_SUFFIX}")
        #endif ()
        #set(VTKm_EXECUTABLE_OUTPUT_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        #set(VTKm_LIBRARY_OUTPUT_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
        set(VTKm_BUILD_CMAKE_BASE_DIR "${CMAKE_BINARY_DIR}")
        #set(VTKm_INSTALL_CONFIG_DIR "${_vtk_build_CMAKE_DESTINATION}/vtkm")
        #set(VTKm_INSTALL_CMAKE_MODULE_DIR "${VTKm_INSTALL_CONFIG_DIR}/cmake")
        # Currently VTK-m only installs its readme and license.
        #set(VTKm_INSTALL_SHARE_DIR "${_vtk_build_LICENSE_DESTINATION}/vtkvtkm")

        #set(VTKm_INSTALL_ONLY_LIBRARIES OFF)

        #set(VTKm_USE_DEFAULT_SYMBOL_VISIBILITY OFF)
        set(VTKm_SKIP_LIBRARY_VERSIONS ON)

        set(VTKm_DEFAULT_TYPES_HEADER "../../../VtkmDefaultTypesVistle.h.in")

        add_subdirectory(vtk-m)
        add_library(vtkm::cont ALIAS vtkm_cont)
        add_library(vtkm::filter ALIAS vtkm_filter)
        #add_library(vtkm_io ALIAS vtkm::io)
        #add_library(vtkm_rendering ALIAS vtkm::rendering)
        #add_library(vtkm_source ALIAS vtkm::source)
        #add_library(vtkm_worklet ALIAS vtkm::worklet)

    endif()

    if(VISTLE_USE_MPI)
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
    set(PythonLibsNew_FIND_REQUIRED FALSE)
    set(PythonLibsNew_FOUND TRUE)
    add_subdirectory(pybind11)
endif()

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

include(cmrc/CMakeRC.cmake)
vistle_export_library(cmrc-base)
