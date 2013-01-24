add_library(vistle_util SHARED
   tools.cpp)

target_link_libraries(vistle_util
        ${Boost_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ..
)
