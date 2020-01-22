set(util_SOURCES
   affinity.cpp
   coRestraint.cpp
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
   pybind.h
   netpbmimage.h
   listenv4v6.h
)

if (NOT CMAKE_BUILD_TYPE STREQUAL "")
   message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
   add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
endif()

vistle_add_library(vistle_util ${VISTLE_LIB_TYPE} ${util_SOURCES} ${util_HEADERS})

target_link_libraries(vistle_util
    PRIVATE ${Boost_SYSTEM_LIBRARY}
)
target_link_libraries(vistle_util
    PUBLIC ${Boost_FILESYSTEM_LIBRARY}
)
target_link_libraries(vistle_util
    PRIVATE Threads::Threads
)

find_library(LIBEXECINFO execinfo)
if (LIBEXECINFO)
    target_link_libraries(vistle_util
        PRIVATE execinfo
    )
endif()
