vistle_find_package(Boost 1.40 REQUIRED COMPONENTS chrono system filesystem)

set(util_SOURCES
   coRestraint.cpp
   exception.cpp
   findself.cpp
   stopwatch.cpp
   tools.cpp
   spawnprocess.cpp
   directory.cpp
   sleep.cpp
)

set(util_HEADERS
   export.h
   coRestraint.h
   exception.h
   findself.h
   sysdep.h
   tools.h
   triple.h
   stopwatch.h
   valgrind.h
   vecstreambuf.h
   spawnprocess.h
   directory.h
   sleep.h
)

vistle_add_library(vistle_util ${VISTLE_LIB_TYPE} ${util_SOURCES} ${util_HEADERS})

target_link_libraries(vistle_util
        ${Boost_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ..
)
