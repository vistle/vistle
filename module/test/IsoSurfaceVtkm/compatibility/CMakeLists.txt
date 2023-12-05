# Check integer compatibility between vistle and vtk-m
try_run(
    RUN_REPORT
    COMPILE_REPORT
    "${CMAKE_BINARY_DIR}/tmp"
    SOURCES
    "${CMAKE_SOURCE_DIR}/module/test/IsoSurfaceVtkm/compatibility/checkIntCompatibility.cpp"
    LINK_LIBRARIES
    vtkm::cont
    ARGS "${VISTLE_64BIT_INDICES}")

if(RUN_REPORT STREQUAL "FAILED_TO_RUN")
    if(VISTLE_64BIT_INDICES)
        message(
            FATAL_ERROR
                "Incompatible integer types!\nVtk-m has been compiled with VTKm_USE_64BIT_IDS=OFF, "
                "i.e., it uses 32-bit integers for indexing. But you are trying to compile vistle to "
                "use 64-bit integers.\nPlease disable VISTLE_64BIT_INDICES.")
    else()
        message(
            FATAL_ERROR
                "Incompatible integer types!\nVtk-m has been compiled with VTKm_USE_64BIT_IDS=ON, "
                "i.e., it uses 64-bit integers for indexing. But you are trying to compile vistle to "
                "use 32-bit integers.\nPlease enable VISTLE_64BIT_INDICES.")
    endif()
endif()

# Check float compatibility between vistle and vtk-m
try_run(
    RUN_REPORT
    COMPILE_REPORT
    "${CMAKE_BINARY_DIR}/tmp"
    SOURCES
    "${CMAKE_SOURCE_DIR}/module/test/IsoSurfaceVtkm/compatibility/checkFloatCompatibility.cpp"
    LINK_LIBRARIES
    vtkm::cont
    ARGS "${VISTLE_DOUBLE_PRECISION}")

if(RUN_REPORT STREQUAL "FAILED_TO_RUN")
    if(VISTLE_DOUBLE_PRECISION)
        message(
            FATAL_ERROR
                "Incompatible floating point types!\nVtk-m has been compiled with " "VTKm_USE_DOUBLE_PRECISION=OFF, i.e., it uses floats. But you are trying "
                "to compile vistle to use double precision.\nPlease disable VISTLE_DOUBLE_PRECISION.")
    else()
        message(
            FATAL_ERROR
                "Incompatible floating point types!\nVtk-m has been compiled with " "VTKm_USE_DOUBLE_PRECISION=ON, i.e., it uses doubles. But you are trying "
                "to compile vistle to using single precision.\nPlease enable VISTLE_DOUBLE_PRECISION.")
    endif()
endif()
