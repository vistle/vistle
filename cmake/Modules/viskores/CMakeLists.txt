# Check integer compatibility between vistle and viskores
try_run(
    RUN_REPORT
    COMPILE_REPORT
    "${CMAKE_BINARY_DIR}/tmp"
    SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/checkIntCompatibility.cpp"
    LINK_LIBRARIES
    viskores::cont
    ARGS "${VISTLE_64BIT_INDICES}")

if(RUN_REPORT STREQUAL "FAILED_TO_RUN")
    get_property(
        helperString
        CACHE VISTLE_64BIT_INDICES
        PROPERTY HELPSTRING)
    if(VISTLE_64BIT_INDICES)
        message(WARNING "To ensure compatibility with viskores VISTLE_64BIT_INDICES will be disabled.")
        set(VISTLE_64BIT_INDICES
            OFF
            CACHE BOOL "${helperString}" FORCE)
    else()
        message(WARNING "To ensure compatibility with viskores VISTLE_64BIT_INDICES will be enabled.")
        set(VISTLE_64BIT_INDICES
            ON
            CACHE BOOL "${helperString}" FORCE)
    endif()
endif()

# Check float compatibility between vistle and viskores
try_run(
    RUN_REPORT
    COMPILE_REPORT
    "${CMAKE_BINARY_DIR}/tmp"
    SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/checkFloatCompatibility.cpp"
    LINK_LIBRARIES
    viskores::cont
    ARGS "${VISTLE_DOUBLE_PRECISION}")

if(RUN_REPORT STREQUAL "FAILED_TO_RUN")
    get_property(
        helperString
        CACHE VISTLE_DOUBLE_PRECISION
        PROPERTY HELPSTRING)
    if(VISTLE_DOUBLE_PRECISION)
        message(WARNING "To ensure compatibility with viskores VISTLE_DOUBLE_PRECISION will be disabled.")
        set(VISTLE_DOUBLE_PRECISION
            OFF
            CACHE BOOL "${helperString}" FORCE)
    else()
        message(WARNING "To ensure compatibility with viskores VISTLE_DOUBLE_PRECISION will be enabled.")
        set(VISTLE_DOUBLE_PRECISION
            ON
            CACHE BOOL "${helperString}" FORCE)
    endif()
endif()
