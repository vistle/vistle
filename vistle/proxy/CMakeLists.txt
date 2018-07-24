vistle_find_package(Boost 1.53 REQUIRED COMPONENTS program_options system)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
)
include_directories(
        ..
)

set(proxy_SOURCES
   proxy.cpp
   )

vistle_add_executable(vistle_proxy ${proxy_SOURCES})
target_link_libraries(vistle_proxy
        ${Boost_LIBRARIES}
        vistle_core
        vistle_net
)
