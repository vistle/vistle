vistle_find_package(BOTAN REQUIRED)

set(util_SOURCES
    affinity.cpp
    coRestraint.cpp
    crypto.cpp
    directory.cpp
    exception.cpp
    fileio.cpp
    findself.cpp
    hostname.cpp
    listenv4v6.cpp
    netpbmimage.cpp
    sleep.cpp
    stopwatch.cpp
    sysdep.cpp
    tools.cpp
    url.cpp
    userinfo.cpp)

set(util_HEADERS
    affinity.h
    allocator.h
    boost_interprocess_config.h
    buffer.h
    coRestraint.h
    crypto.h
    directory.h
    enum.h
    enumarray.h
    exception.h
    export.h
    fileio.h
    findself.h
    hostname.h
    listenv4v6.h
    math.h
    netpbmimage.h
    pybind.h
    sleep.h
    ssize_t.h
    stopwatch.h
    sysdep.h
    tools.h
    url.h
    userinfo.h
    valgrind.h
    vecstreambuf.h)

if(NOT CMAKE_BUILD_TYPE STREQUAL "")
    message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
    add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
endif()

vistle_add_library(vistle_util EXPORT ${VISTLE_LIB_TYPE} ${util_SOURCES} ${util_HEADERS})

target_include_directories(vistle_util SYSTEM PRIVATE ${BOTAN_INCLUDE_DIRS})
vistle_target_link_libraries(
    vistle_util
    PRIVATE
    Boost::system
    PUBLIC
    Boost::filesystem
    PRIVATE
    Threads::Threads
    PRIVATE
    ${BOTAN_LIBRARIES})

find_library(LIBEXECINFO execinfo)
if(LIBEXECINFO)
    vistle_target_link_libraries(vistle_util PRIVATE execinfo)
endif()

set(util_mpi_SOURCES shmconfig.cpp)

set(_util_mpi_SOURCES shmconfig.h)

set(visibility INTERNAL)
if(NOT VISTLE_BUILD_SHARED)
    set(visibility EXPORT)
endif()
vistle_add_library(vistle_util_mpi EXPORT STATIC ${util_mpi_SOURCES} ${util_mpi_HEADERS})
target_include_directories(vistle_util SYSTEM PRIVATE ${MPI_INCLUDE_DIRS})
vistle_target_link_libraries(vistle_util_mpi ${BOOST_MPI})
set_target_properties(vistle_util_mpi PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
