vistle_find_package(TBB)
vistle_find_package(JPEGTURBO)
vistle_find_package(GLEW)
vistle_find_package(Boost 1.40 REQUIRED)
vistle_find_package(ZFP)
using(SNAPPY optional)
using(GLEW)
using(BOOST)
if(NOT JPEGTURBO_FOUND)
   RETURN()
endif(NOT JPEGTURBO_FOUND)
if(NOT TURBOJPEG_HAVE_TJMCUWIDTH)
   return()
endif()

if(NOT TBB_FOUND)
   return()
endif()

if(JPEGTURBO_FOUND)
   add_definitions(-DHAVE_TURBOJPEG)
endif()

if(ZFP_FOUND)
   add_definitions(-DHAVE_ZFP)
   include_directories(${ZFP_INCLUDE_DIRS})
endif()


include_directories(
   .. # VistlePluginUtil
   ${PROJECT_SOURCE_DIR}
   ${TBB_INCLUDE_DIR}
   ${JPEGTURBO_INCLUDE_DIR}
   ${Boost_INCLUDE_DIRS}
)

set(HEADERS
  RhrClient.h
)

set(SOURCES
  RhrClient.cpp
)

#use_openmp()

if(NOT WIN32)
   set_source_files_properties(RhrClient.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif()

cover_add_plugin(RhrClient ${HEADER} ${SOURCES})
target_link_libraries(RhrClient VistlePluginUtil ${COVISE_NET_LIBRARY} vistle_rhr ${TURBOJPEG_LIBRARIES} ${TBB_LIBRARIES} ${Boost_LIBRARIES})
if(UNIX AND NOT APPLE)
   target_link_libraries(RhrClient
        -lpthread                                                                                                                                                                                                                                                                           
   )
endif()
if (ZFP_FOUND)
    target_link_libraries(RhrClient ${ZFP_LIBRARIES})
endif()

