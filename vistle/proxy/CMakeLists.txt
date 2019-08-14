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
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        vistle_core
        vistle_net
)
