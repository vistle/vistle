# Checks if integers that vtkm use are compatible with the integers vistle uses.

if(VISTLE_64BIT_INDICES)
    set(VISTLE_IS_USING_64_BIT "1")
else()
    set(VISTLE_IS_USING_64_BIT "0")
endif()

try_run(RUN_REPORT
        COMPILE_REPORT
        "${CMAKE_BINARY_DIR}/tmp"
        SOURCES "${CMAKE_SOURCE_DIR}/module/test/IsoSurfaceVtkm/compatibility/checkIntCompatibility.cpp"
        LINK_LIBRARIES vtkm::cont
        ARGS "${VISTLE_IS_USING_64_BIT}")

if(RUN_REPORT STREQUAL "FAILED_TO_RUN")
    if(VISTLE_64BIT_INDICES)
        message(FATAL_ERROR "Incompatible integer types!\nVtk-m has been compiled with VTKm_USE_64BIT_IDS=OFF, "
                            "i.e., it uses 32-bit integers for indexing. But you are trying to compile vistle to "
                            "use 64-bit integers.\nPlease disable VISTLE_64BIT_INDICES.")
    else()
        message(FATAL_ERROR "Incompatible integer types!\nVtk-m has been compiled with VTKm_USE_64BIT_IDS=ON, "
                            "i.e., it uses 64-bit integers for indexing. But you are trying to compile vistle to "
                            "use 32-bit integers.\nPlease enable VISTLE_64BIT_INDICES.")
    endif()
endif()
