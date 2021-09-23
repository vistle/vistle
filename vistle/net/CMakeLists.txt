set(net_SOURCES dataproxy.cpp tunnel.cpp)

set(net_HEADERS export.h dataproxy.h tunnel.h)

vistle_add_library(vistle_net ${VISTLE_LIB_TYPE} ${net_SOURCES} ${net_HEADERS})
vistle_export_library(vistle_net ${VISTLE_LIB_TYPE} ${net_SOURCES} ${net_HEADERS})

target_link_libraries(
    vistle_net
    PRIVATE Boost::system
    PRIVATE vistle_util
    PRIVATE vistle_core
    PRIVATE Threads::Threads)
