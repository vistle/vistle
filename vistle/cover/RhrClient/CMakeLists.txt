vistle_find_package(TBB)
vistle_find_package(JPEGTURBO)
vistle_find_package(GLEW)
vistle_find_package(Boost 1.53 REQUIRED)
vistle_find_package(ZFP)
vistle_find_package(SNAPPY)
using(SNAPPY optional)
using(GLEW)

if(NOT TBB_FOUND)
   return()
endif()

if(JPEGTURBO_FOUND AND TURBOJPEG_HAVE_TJMCUWIDTH)
   add_definitions(-DHAVE_TURBOJPEG)
   include_directories(SYSTEM ${JPEGTURBO_INCLUDE_DIR})
endif()

if(ZFP_FOUND)
   add_definitions(-DHAVE_ZFP)
   include_directories(SYSTEM ${ZFP_INCLUDE_DIRS})
endif()


include_directories(
   .. # VistlePluginUtil
)
include_directories(SYSTEM
   ${TBB_INCLUDE_DIR}
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
if(JPEGTURBO_FOUND AND TURBOJPEG_HAVE_TJMCUWIDTH)
   target_link_libraries(RhrClient ${TURBOJPEG_LIBRARIES})
endif()

