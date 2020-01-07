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
    PRIVATE ${Boost_PROGRAM_OPTIONS_LIBRARY}
    PRIVATE ${Boost_SYSTEM_LIBRARY}
    PRIVATE vistle_util
    PRIVATE vistle_core
    PRIVATE vistle_net
)

target_link_libraries(vistle_proxy
    PRIVATE Threads::Threads
)
