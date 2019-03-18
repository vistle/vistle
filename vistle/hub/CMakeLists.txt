vistle_find_package(Boost 1.53 REQUIRED COMPONENTS program_options system)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
)
include_directories(
        ..
)

vistle_add_executable(vistle hubmain.cpp)
target_link_libraries(vistle
        ${Boost_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_control
        vistle_util
        vistle_core
        vistle_net
)
