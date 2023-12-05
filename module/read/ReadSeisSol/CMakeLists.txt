if(NOT Xdmf_FOUND)
    return()
endif()

find_package(HDF5 COMPONENTS C)
find_package(Iconv)

set(HEADERS ReadSeisSol.h surfacetools.h)
set(SOURCES ReadSeisSol.cpp surfacetools.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_module(ReadSeisSol "Read SeisSol data files (XDMF/HDF5)" ${SOURCES} ${HEADERS})
if(MSVC)
    target_link_libraries(
        ReadSeisSol
        vistle_xdmf
        debug
        XdmfCored
        optimized
        XdmfCore
        optimized
        ${XDMF_LIBNAME}
        debug
        ${XDMF_LIBNAME}d
        ${HDF5_LIBRARIES}
        ${Iconv_LIBRARIES})
else()
    target_link_libraries(ReadSeisSol vistle_xdmf XdmfCore ${XDMF_LIBNAME} ${HDF5_LIBRARIES})
endif()
target_include_directories(ReadSeisSol PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS} ${XDMF_Iconv_INCLUDE_DIRS})

find_program(PYINSTALLER pyinstaller)
if(NOT PYINSTALLER)
    return()
endif()

set(VISTLE_PYINSTALLER_COMPILE
    pyinstaller
    --noconfirm
    --console
    --distpath
    ${EXECUTABLE_OUTPUT_PATH}
    --onefile
    --name)

set(SEISSOL_RESAMPLE_XDMF_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/utils/recreateXdmf.py)
set(SEISSOL_RESAMPLE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/utils/extractDataFromUnstructuredOutput.py)
set(SEISSOL_RESAMPLE_XDMF_BIN ${CMAKE_CURRENT_BINARY_DIR}/utils/seissol_resample_xdmf)
set(SEISSOL_RESAMPLE_BIN ${CMAKE_CURRENT_BINARY_DIR}/utils/seissol_resample)

add_custom_target(build_seissol_resample_xdmf ALL DEPENDS ${SEISSOL_RESAMPLE_XDMF_BIN})
add_custom_target(build_seissol_resample ALL DEPENDS "${SEISSOL_RESAMPLE_BIN}")

add_custom_command(
    OUTPUT ${SEISSOL_RESAMPLE_XDMF_BIN}
    COMMENT "Create seissol_resample_xdmf"
    COMMAND ${VISTLE_PYINSTALLER_COMPILE} seissol_resample_xdmf ${SEISSOL_RESAMPLE_XDMF_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E touch ${SEISSOL_RESAMPLE_XDMF_BIN}
    DEPENDS ${SEISSOL_RESAMPLE_XDMF_SOURCE}
    VERBATIM)

add_custom_command(
    OUTPUT ${SEISSOL_RESAMPLE_BIN}
    COMMENT "Create seissol_resample"
    COMMAND ${VISTLE_PYINSTALLER_COMPILE} seissol_resample ${SEISSOL_RESAMPLE_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E touch ${SEISSOL_RESAMPLE_BIN}
    DEPENDS ${SEISSOL_RESAMPLE_SOURCE}
    VERBATIM)
use_openmp()
