vistle_find_package(BISON 3.8)
vistle_find_package(FLEX)

if(NOT BISON_FOUND)
    message("ReadEnsight: cannot build without bison 3.8 or newer")
    return()
endif()
if(NOT FLEX_FOUND)
    message("ReadEnsight: cannot build without flex")
    return()
endif()

if(NOT FLEX_INCLUDE_DIR)
    message("ReadEnsight: cannot build without FLEX_INCLUDE_DIR")
    return()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
include_directories(SYSTEM ${FLEX_INCLUDE_DIRS})

set(HEADERS
    ByteSwap.h
    CaseFile.h
    CaseLexer.h
    CaseParserDriver.h
    DataGoldAscii.h
    DataGoldBin.h
    DataItem.h
    EnElement.h
    EnFile.h
    EnPart.h
    GeoGoldAscii.h
    GeoGoldBin.h
    ReadEnsight.h)

set(SOURCES
    CaseFile.cpp
    CaseParserDriver.cpp
    DataGoldAscii.cpp
    DataGoldBin.cpp
    DataItem.cpp
    EnElement.cpp
    EnFile.cpp
    EnPart.cpp
    GeoGoldAscii.cpp
    GeoGoldBin.cpp
    ReadEnsight.cpp)

set(EXTRASOURCES CaseParser.y CaseLexer.l)

bison_target(
    CaseParser CaseParser.y ${CMAKE_CURRENT_BINARY_DIR}/CaseParser.cpp
    COMPILE_FLAGS --header=${CMAKE_CURRENT_BINARY_DIR}/CaseParser.h
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/CaseParser.h)
flex_target(CaseScanner CaseLexer.l ${CMAKE_CURRENT_BINARY_DIR}/CaseLexer.cpp)
add_flex_bison_dependency(CaseScanner CaseParser)

add_module(
    ReadEnsight
    "read EnSight Gold data"
    ${SOURCES}
    ${HEADERS}
    ${EXTRASOURCES}
    ${BISON_CaseParser_OUTPUT_SOURCE}
    ${FLEX_CaseScanner_OUTPUTS})
