vistle_find_package(BOTAN REQUIRED)

set(util_SOURCES
   affinity.cpp
   coRestraint.cpp
   crypto.cpp
   exception.cpp
   fileio.cpp
   findself.cpp
   hostname.cpp
   stopwatch.cpp
   tools.cpp
   spawnprocess.cpp
   directory.cpp
   sleep.cpp
   sysdep.cpp
   netpbmimage.cpp
   listenv4v6.cpp
)

set(util_HEADERS
   export.h
   affinity.h
   coRestraint.h
   crypto.h
   enum.h
   exception.h
   fileio.h
   findself.h
   hostname.h
   math.h
   sysdep.h
   tools.h
   stopwatch.h
   valgrind.h
   vecstreambuf.h
   spawnprocess.h
   directory.h
   sleep.h
   ssize_t.h
   pybind.h
   netpbmimage.h
   listenv4v6.h
)

if (NOT CMAKE_BUILD_TYPE STREQUAL "")
   message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
   add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
endif()

vistle_add_library(vistle_util ${VISTLE_LIB_TYPE} ${util_SOURCES} ${util_HEADERS})
vistle_export_library(vistle_util ${VISTLE_LIB_TYPE} ${util_SOURCES} ${util_HEADERS})

target_include_directories(vistle_util SYSTEM PRIVATE ${BOTAN_INCLUDE_DIRS})

vistle_link_libraries(vistle_util
    PRIVATE Boost::system
	PUBLIC Boost::filesystem
	PRIVATE Threads::Threads
	PRIVATE ${BOTAN_LIBRARIES}
)


find_library(LIBEXECINFO execinfo)
if (LIBEXECINFO)
    vistle_link_libraries(vistle_util
        PRIVATE execinfo
    )
endif()
