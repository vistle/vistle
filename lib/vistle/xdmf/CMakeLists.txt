if(NOT Xdmf_FOUND)
    return()
endif()

set(xdmf_SOURCES xdmf.cpp)
set(xdmf_HEADERS xdmf.h)

vistle_add_library(vistle_xdmf ${VISTLE_LIB_TYPE} ${xdmf_SOURCES} ${xdmf_HEADERS})
vistle_export_library(vistle_xdmf ${VISTLE_LIB_TYPE} ${xdmf_SOURCES} ${xdmf_HEADERS})
target_include_directories(vistle_xdmf SYSTEM PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS})
vistle_target_link_libraries(vistle_xdmf XdmfCore ${XDMF_LIBNAME})
