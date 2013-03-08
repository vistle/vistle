find_package(Boost 1.40 REQUIRED COMPONENTS serialization)

add_library(vistle_core SHARED
   exception.cpp
   shm.cpp
   objectcache.cpp
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
   module.cpp
   renderer.cpp
   parameter.cpp
   paramvector.cpp
   vector.cpp)

if(UNIX AND NOT APPLE)
	target_link_libraries(vistle_core
		-lrt
	)
endif()

target_link_libraries(vistle_core
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ..
)
