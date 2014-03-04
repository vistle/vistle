find_package(Boost 1.40 REQUIRED COMPONENTS chrono system)

set(util_SOURCES
   coRestraint.cpp
   findself.cpp
   stopwatch.cpp
   tools.cpp
)

set(util_HEADERS
   export.h
   coRestraint.h
   findself.h
   sysdep.h
   tools.h
   triple.h
   stopwatch.h
   valgrind.h
   vecstreambuf.h
)

add_library(vistle_util ${VISTLE_LIB_TYPE} ${util_SOURCES} ${util_HEADERS})

target_link_libraries(vistle_util
        ${Boost_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ..
)
