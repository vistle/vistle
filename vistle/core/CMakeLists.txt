vistle_find_package(Zstd)
vistle_find_package(LZ4)
vistle_find_package(SNAPPY)

if(ZFP_FOUND)
   add_definitions(-DHAVE_ZFP)
   include_directories(SYSTEM ${ZFP_INCLUDE_DIRS})
endif()

if (ZSTD_FOUND)
   add_definitions(-DHAVE_ZSTD)
   include_directories(SYSTEM ${ZSTD_INCLUDE_DIRS})
endif()

if (LZ4_FOUND)
   add_definitions(-DHAVE_LZ4)
   include_directories(SYSTEM ${LZ4_INCLUDE_DIRS})
endif()

if (SNAPPY_FOUND)
   add_definitions(-DHAVE_SNAPPY)
   include_directories(SYSTEM ${SNAPPY_INCLUDE_DIRS})
endif()

set(core_SOURCES
   archives.cpp
   archive_loader.cpp
   archive_saver.cpp
   shm.cpp
   shmname.cpp
   shm_reference.cpp
   shm_obj_ref.cpp
   objectmeta.cpp
   object.cpp
   allobjects.cpp # just one file including all the others for faster compilation
   message.cpp
   messages.cpp
   messagequeue.cpp
   messagerouter.cpp
   messagepayload.cpp
   tcpmessage.cpp
   filequery.cpp
   port.cpp
   parameter.cpp
   parametermanager.cpp
   paramvector.cpp
   vector.cpp
   porttracker.cpp
   statetracker.cpp
   findobjectreferenceoarchive.cpp
   cellalgorithm.cpp
   geometry.cpp
)

if(${TEMPLATES_IN_HEADERS})
   # but just one file including all the others for faster compilation
   set(core_SOURCES ${core_SOURCES}
      vec.cpp
      database.cpp
      celltree.cpp
      vertexownerlist.cpp
      coords.cpp
      coordswradius.cpp
      normals.cpp
      points.cpp
      spheres.cpp
      tubes.cpp
      indexed.cpp
      lines.cpp
      ngons.cpp
      polygons.cpp
      texture1d.cpp
      empty.cpp
      placeholder.cpp
      unstr.cpp
      structuredgridbase.cpp
      uniformgrid.cpp
      rectilineargrid.cpp
      structuredgrid.cpp
      grid.cpp
   )
endif()

set(core_HEADERS
   assert.h
   archives.h
   archives_config.h
   archive_loader.h
   archive_saver.h
   celltree.h
   celltree_impl.h
   vertexownerlist.h
   vertexownerlist_impl.h
   coords.h
   coords_impl.h
   coordswradius.h
   coordswradius_impl.h
   dimensions.h
   export.h
   geometry.h
   index.h
   indexed.h
   indexed_impl.h
   lines.h
   lines_impl.h
   message.h
   messages.h
   messagequeue.h
   messagerouter.h
   messagesender.h
   messagepayload.h
   normals.h
   normals_impl.h
   object.h
   object_impl.h
   objectmeta.h
   objectmeta_impl.h
   parameter.h
   parametermanager.h
   parametermanager_impl.h
   paramvector.h
   paramvector_impl.h
   empty.h
   empty_impl.h
   placeholder.h
   placeholder_impl.h
   points.h
   points_impl.h
   spheres.h
   spheres_impl.h
   tubes.h
   tubes_impl.h
   polygons.h
   polygons_impl.h
   port.h
   porttracker.h
   rectilineargrid.h
   rectilineargrid_impl.h
   scalar.h
   scalars.h
   serialize.h
   shm.h
   shmname.h
   shmvector.h
   shm_array.h
   shm_impl.h
   shm_reference.h
   shm_reference_impl.h
   shm_obj_ref.h
   findobjectreferenceoarchive.h
   structuredgrid.h
   structuredgrid_impl.h
   structuredgridbase.h
   structuredgridbase_impl.h
   statetracker.h
   tcpmessage.h
   filequery.h
   texture1d.h
   texture1d_impl.h
   triangles.h
   quads.h
   ngons.h
   ngons_impl.h
   unstr.h
   unstr_impl.h
   database.h
   database_impl.h
   vec.h
   vec_impl.h
   vec_template.h
   vector.h
   uniformgrid.h
   uniformgrid_impl.h
   uuid.h
   grid.h
   cellalgorithm.h
)
if (MSVC)
  set_source_files_properties(unstr.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

if (VISTLE_SHM_PER_RANK)
   add_definitions(-DSHMPERRANK)
endif()

vistle_add_library(vistle_core ${VISTLE_LIB_TYPE} ${core_SOURCES} ${core_HEADERS})

if(UNIX AND NOT APPLE)
	target_link_libraries(vistle_core
		-lrt
	)
endif()
if (WIN32)
	target_link_libraries(vistle_core
	       	bcrypt.lib
	)
endif()

target_link_libraries(vistle_core
        vistle_util
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_SERIALIZATION_LIBRARY}
        ${Boost_DATE_TIME_LIBRARY}
)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
)
include_directories(
        ..
)

if (ZFP_FOUND)
    target_link_libraries(vistle_core ${ZFP_LIBRARIES})
endif()

if (ZSTD_FOUND)
    target_link_libraries(vistle_core ${ZSTD_LIBRARIES})
endif()

if (LZ4_FOUND)
    target_link_libraries(vistle_core ${LZ4_LIBRARIES})
endif()

if (SNAPPY_FOUND)
    target_link_libraries(vistle_core ${SNAPPY_LIBRARIES})
endif()
