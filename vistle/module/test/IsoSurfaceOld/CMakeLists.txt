use_openmp()

include_directories(SYSTEM
        ${THRUST_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
)

add_module(IsoSurfaceOld IsoSurface.cpp)

target_link_libraries(IsoSurfaceOld
        ${TBB_LIBRARIES}
)

