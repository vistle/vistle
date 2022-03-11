if(NOT Xdmf_FOUND)
    return()
endif()

set(HEADERS ReadSeisSol.h surfacetools.h)
set(SOURCES ReadSeisSol.cpp surfacetools.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_module(ReadSeisSol "read ChEESE seismic data files (XDMF/HDF5)" ${SOURCES} ${HEADERS})

target_link_libraries(ReadSeisSol vistle_xdmf XdmfCore ${XDMF_LIBNAME})
target_include_directories(ReadSeisSol PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS})

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

add_custom_command(
    TARGET ReadSeisSol
    POST_BUILD
    COMMAND ${VISTLE_PYINSTALLER_COMPILE} resample_seissol_xdmf ${CMAKE_CURRENT_SOURCE_DIR}/utils/recreateXdmf.py
    VERBATIM)

add_custom_command(
    TARGET ReadSeisSol
    POST_BUILD
    COMMAND ${VISTLE_PYINSTALLER_COMPILE} resample_seissol ${CMAKE_CURRENT_SOURCE_DIR}/utils/extractDataFromUnstructuredOutput.py
    VERBATIM)
