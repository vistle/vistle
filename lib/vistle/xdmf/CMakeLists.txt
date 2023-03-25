if(NOT Xdmf_FOUND)
    return()
endif()

set(xdmf_SOURCES xdmf.cpp)
set(xdmf_HEADERS xdmf.h)

vistle_add_library(vistle_xdmf INTERNAL STATIC ${xdmf_SOURCES} ${xdmf_HEADERS})

target_include_directories(vistle_xdmf SYSTEM PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS})
if(WIN32)
    target_include_directories(vistle_xdmf SYSTEM PRIVATE ${Iconv_INCLUDE_DIRS})
endif()

if(MSVC)
vistle_target_link_libraries(vistle_xdmf ${XDMF_CMAKE_LIBRARY_PATH_FLAG}${XDMF_CMAKE_INSTALL_FULL_LIBDIR} debug XdmfCored optimized XdmfCore optimized ${XDMF_LIBNAME} debug ${XDMF_LIBNAME}d)
else()
vistle_target_link_libraries(vistle_xdmf ${XDMF_CMAKE_LIBRARY_PATH_FLAG}${XDMF_CMAKE_INSTALL_FULL_LIBDIR} XdmfCore ${XDMF_LIBNAME})
endif()
