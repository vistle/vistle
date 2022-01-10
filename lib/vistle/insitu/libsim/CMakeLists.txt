option(LIBSIM_DEBUG "print debug messages" OFF)

add_subdirectory(engineInterface)
add_subdirectory(connectLibsim)
#add_subdirectory(moduleOption)
include(libsimInterface/libsim_interface.cmake)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

function(createSimV2Lib libName)
    set(RUNTIME_SOURCES
        DataTransmitter.cpp
        Engine.cpp
        Exception.cpp
        MetaData.cpp
        RectilinearMesh.cpp
        StructuredMesh.cpp
        UnstructuredMesh.cpp
        VariableData.cpp
        VertexTypesToVistle.cpp
        VisItControlInterfaceRuntime.C)

    set(RUNTIME_HEADERS
        ArrayStruct.h
        DataTransmitter.h
        Engine.h
        Exception.h
        MeshInfo.h
        MetaData.h
        RectilinearMesh.h
        SmartHandle.h
        StructuredMesh.h
        UnstructuredMesh.h
        VariableData.h
        VariableInfo.h
        VertexTypesToVistle.h
        VisItControlInterfaceRuntime.h
        VisitDataTypesToVistle.h
        export.h)

    vistle_add_library(${libName} ${RUNTIME_HEADERS} ${RUNTIME_SOURCES} ${LIBSIM_INTERFACE_FILES})
    vistle_export_library(${libName} ${RUNTIME_HEADERS} ${RUNTIME_SOURCES})

    if(LIBSIM_DEBUG)
        target_compile_definitions(${libName} PRIVATE LIBSIM_DEBUG)
    endif()
    target_compile_definitions(${libName} PRIVATE LIBSIM_EXPORT)

    # By default, symbols provided by the sublibs that are not used by mainlib (which is all of them in this case)
    # are not used. This changes that.
    if(NOT WIN32)
        add_definitions("-fPIC")
    endif()

    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -fPIC")
    endif()

    target_compile_features(${libName} PRIVATE cxx_std_14)

    vistle_target_link_libraries(
        ${libName}
        PRIVATE
        Boost::boost
        PRIVATE
        vistle_insitu_core
        PRIVATE
        vistle_insitu_message
        PRIVATE
        vistle_module
        PRIVATE
        vistle_util
        PRIVATE
        vistle_util_mpi
        PRIVATE
        MPI::MPI_C
        PRIVATE
        Threads::Threads
        PRIVATE
        ${BOOST_MPI}
        PRIVATE
        vistle_core)

    if(NOT VISTLE_MULTI_PROCESS)
        target_link_libraries(
            ${libName}
            PRIVATE vistle_clustermanager
            PRIVATE vistle_libsim_engine_interface)
    endif()
endfunction(createSimV2Lib)

set(LIBPREFIX lib) #export.h has to be adopted if this changes

if(NOT MSVC)
    set(LIBPREFIX "") #to compensate the additional lib prefix from vistle_add_library
endif()

#libsim uses a optimized non parallel library. For Vistle both libraries are the same but with different names
#so that libsim can locate them
createsimv2lib(${LIBPREFIX}simV2runtime_par)
createsimv2lib(${LIBPREFIX}simV2runtime_ser)

vistle_install_docs(README.md)
