set(alg_SOURCES objalg.cpp fields.cpp)
set(alg_HEADERS export.h objalg.h geo.h ghost.h fields.h)

vistle_add_library(vistle_alg EXPORT ${alg_SOURCES} ${alg_HEADERS})
target_link_libraries(vistle_alg PRIVATE vistle_core)
