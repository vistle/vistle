vistle_find_package(Zstd)
vistle_find_package(LZ4)
vistle_find_package(SNAPPY)



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
   messagesender.cpp
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
   archives.h
   archives_config.h
   archive_loader.h
   archive_saver.h
   celltree.h
   celltree_impl.h
   vertexownerlist.h
   vertexownerlist_impl.h
   celltreenode.h
   celltypes.h
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
   shmdata.h
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
  set_source_files_properties(vec.cpp PROPERTIES COMPILE_FLAGS /bigobj)
  set_source_files_properties(cellalgorithm.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()



vistle_add_library(vistle_core ${VISTLE_LIB_TYPE} ${core_SOURCES} ${core_HEADERS})
vistle_export_library(vistle_core ${VISTLE_LIB_TYPE} ${core_SOURCES} ${core_HEADERS})
#vistle_install_headers(core ${core_HEADERS})

if(${TEMPLATES_IN_HEADERS})
   target_compile_definitions(vistle_core PUBLIC -DTEMPLATES_IN_HEADERS)
   target_compile_definitions(vistle_core PUBLIC -DVISTLE_IMPL)
endif()

if(UNIX AND NOT APPLE)
	target_link_libraries(vistle_core
        PUBLIC -lrt
	)
endif()
if (WIN32)
	target_link_libraries(vistle_core
	      PUBLIC bcrypt.lib
	)
endif()

vistle_target_link_libraries(vistle_core
        PRIVATE vistle_util
        PUBLIC vistle_boost_mpi
        PUBLIC Boost::system
        PUBLIC Boost::serialization
        PUBLIC Boost::date_time
		PRIVATE Threads::Threads
)

get_target_property(vistle_boost_mpi_includes vistle_boost_mpi INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(vistle_core PRIVATE ${vistle_boost_mpi_includes})

if (VISTLE_SHM_PER_RANK)
   target_compile_definitions(vistle_core PRIVATE -DSHMPERRANK)
endif()
if(ZFP_FOUND)

   target_compile_definitions(vistle_core PRIVATE -DHAVE_ZFP)
   target_include_directories(vistle_core SYSTEM PRIVATE ${ZFP_INCLUDE_DIRS})
   vistle_target_link_libraries(vistle_core PRIVATE ${ZFP_LIBRARIES})
   if(VISTLE_INSTALL_3RDPARTY)
       install(
	    DIRECTORY 
		     ${PROJECT_SOURCE_DIR}/3rdparty/zfp/include/
	    DESTINATION
		    include/3rdarty/
	      COMPONENT
		    Devel
    )
    target_include_directories(vistle_core SYSTEM
		    PUBLIC $<INSTALL_INTERFACE:include/3rdarty>
    )
    endif()
endif()

if (ZSTD_FOUND)
   target_compile_definitions(vistle_core PRIVATE -DHAVE_ZSTD)
   target_include_directories(vistle_core SYSTEM PRIVATE ${ZSTD_INCLUDE_DIRS})
   vistle_target_link_libraries(vistle_core PRIVATE ${ZSTD_LIBRARIES})
endif()

if (LZ4_FOUND)
   target_compile_definitions(vistle_core PRIVATE -DHAVE_LZ4)
   target_include_directories(vistle_core SYSTEM PRIVATE ${LZ4_INCLUDE_DIRS})
   vistle_target_link_libraries(vistle_core PRIVATE ${LZ4_LIBRARIES})
endif()

if (SNAPPY_FOUND)
   target_compile_definitions(vistle_core PRIVATE -DHAVE_SNAPPY)
   target_include_directories(vistle_core SYSTEM PRIVATE ${SNAPPY_INCLUDE_DIRS})
   vistle_target_link_libraries(vistle_core PRIVATE ${SNAPPY_LIBRARIES})
endif()

target_include_directories(vistle_core SYSTEM
        PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdparty>
)
target_include_directories(vistle_core SYSTEM
        PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdparty/yas/include>
)

if(VISTLE_INSTALL_3RDPARTY)
    #yas
    install(
	    DIRECTORY 
		     ${PROJECT_SOURCE_DIR}/3rdparty/yas/include/yas
	    DESTINATION
		    include/3rdarty/
	      COMPONENT
		    Devel
    )
    target_include_directories(vistle_core SYSTEM
		    PUBLIC $<INSTALL_INTERFACE:include/3rdarty>
    )
    #eigen3
    install(
	    DIRECTORY 
		     ${PROJECT_SOURCE_DIR}/3rdparty/eigen3
	    DESTINATION
		    include/3rdarty/
	      COMPONENT
		    Devel
    )
    target_include_directories(vistle_core SYSTEM
		    PUBLIC $<INSTALL_INTERFACE:include/3rdarty>
    )
endif()

