use_openmp()

include_directories(SYSTEM ${THRUST_INCLUDE_DIR} ${TBB_INCLUDE_DIRS})

add_module(IsoSurfaceOld "compute iso surfaces on structured grids, deprecated" IsoSurface.cpp)

target_link_libraries(IsoSurfaceOld ${TBB_LIBRARIES})
