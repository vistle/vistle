include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
)
include_directories(
        ..
)

vistle_add_executable(vistle hubmain.cpp)
target_link_libraries(vistle
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${EXTRA_LIBRARIES}
        vistle_control
        vistle_util
        vistle_core
        vistle_net
)
