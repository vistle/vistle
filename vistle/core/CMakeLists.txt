find_package(Boost 1.40 REQUIRED COMPONENTS serialization system)

set(core_SOURCES
   exception.cpp
   shm.cpp
   objectcache.cpp
   objectmeta.cpp
   object.cpp
   allobjects.cpp # just one file including all the others for faster compilation
   #vec.cpp
   #coords.cpp
   #points.cpp
   #indexed.cpp
   #lines.cpp
   #triangles.cpp
   #polygons.cpp
   #unstr.cpp
   #set.cpp
   #geometry.cpp
   #texture1d.cpp
   message.cpp
   messagequeue.cpp
   tcpmessage.cpp
   port.cpp
   parameter.cpp
   paramvector.cpp
   vector.cpp
   porttracker.cpp
   statetracker.cpp
)

set(core_HEADERS
   archives.h
   coords.h
   coords_impl.h
   dimensions.h
   exception.h
   export.h
   geometry.h
   geometry_impl.h
   index.h
   indexed.h
   indexed_impl.h
   lines.h
   lines_impl.h
   message.h
   messagequeue.h
   normals.h
   object.h
   object_impl.h
   objectcache.h
   objectmeta.h
   objectmeta_impl.h
   parameter.h
   paramvector.h
   paramvector_impl.h
   placeholder.h
   placeholder_impl.h
   points.h
   points_impl.h
   polygons.h
   polygons_impl.h
   port.h
   porttracker.h
   scalar.h
   scalars.h
   serialize.h
   shm.h
   shm_impl.h
   statetracker.h
   tcpmessage.h
   texture1d.h
   texture1d_impl.h
   triangles.h
   triangles_impl.h
   unstr.h
   unstr_impl.h
   vec.h
   vec_impl.h
   vector.h
)

add_library(vistle_core ${VISTLE_LIB_TYPE} ${core_SOURCES} ${core_HEADERS})

if(UNIX AND NOT APPLE)
	target_link_libraries(vistle_core
		-lrt
	)
endif()

target_link_libraries(vistle_core
        vistle_util
        ${Boost_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ..
)
