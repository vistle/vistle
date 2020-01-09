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
    PRIVATE ${Boost_SYSTEM_LIBRARY}
    PRIVATE vistle_util
    PRIVATE vistle_core
)

target_link_libraries(vistle_net
    PRIVATE Threads::Threads
)
