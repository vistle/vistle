set(userinterface_SOURCES userinterface.cpp vistleconnection.cpp)
set(userinterface_HEADERS export.h userinterface.h vistleconnection.h)

vistle_add_library(vistle_userinterface EXPORT ${VISTLE_LIB_TYPE} ${userinterface_SOURCES} ${userinterface_HEADERS})
target_link_libraries(
    vistle_userinterface
    PRIVATE Boost::system
    PRIVATE vistle_core
    PRIVATE vistle_util
    PRIVATE vistle_python
    PRIVATE Threads::Threads)
