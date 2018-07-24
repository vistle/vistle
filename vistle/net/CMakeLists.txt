vistle_find_package(Boost 1.53 REQUIRED COMPONENTS system)

set(net_SOURCES
   dataproxy.cpp
   tunnel.cpp
)

set(net_HEADERS
   export.h
   dataproxy.h
   tunnel.h
)

vistle_add_library(vistle_net ${VISTLE_LIB_TYPE} ${net_SOURCES} ${net_HEADERS})

target_link_libraries(vistle_net
        ${Boost_LIBRARIES}
        vistle_core
)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
)
include_directories(
        ..
)

