#project(vistle_3rdparty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

set(CMAKE_C_VISIBILITY_PRESET default)
set(CMAKE_CXX_VISIBILITY_PRESET default)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996 /wd4267 /wd4800 /wd4244")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996 /wd4267 /wd4800 /wd4244")
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

if(NOT VISTLE_GUI_ONLY)
    #set(BUILD_SHARED_LIBS ${VISTLE_BUILD_SHARED} CACHE INTERNAL "")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
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
