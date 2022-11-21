if(NOT Xdmf_FOUND)
    return()
endif()

if(WIN32)
    vistle_find_package(iconv REQUIRED)
endif()

set(xdmf_SOURCES xdmf.cpp)
set(xdmf_HEADERS xdmf.h)

vistle_add_library(vistle_xdmf INTERNAL STATIC ${xdmf_SOURCES} ${xdmf_HEADERS})

if(NOT WIN32)
    target_include_directories(vistle_xdmf SYSTEM PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS})
else()
    target_include_directories(vistle_xdmf SYSTEM PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS} ${Iconv_INCLUDE_DIRS})
endif()

vistle_target_link_libraries(vistle_xdmf ${XDMF_CMAKE_LIBRARY_PATH_FLAG}${XDMF_CMAKE_INSTALL_FULL_LIBDIR} XdmfCore ${XDMF_LIBNAME})
