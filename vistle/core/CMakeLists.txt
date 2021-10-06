vistle_find_package(LZ4)
vistle_find_package(SNAPPY)
vistle_find_package(ZSTD)

set(core_SOURCES
    allobjects.cpp # just one file including all the others for faster compilation
    availablemodule.cpp
    archive_loader.cpp
    archive_saver.cpp
    archives.cpp
    cellalgorithm.cpp
    filequery.cpp
    findobjectreferenceoarchive.cpp
    geometry.cpp
    message.cpp
    messagepayload.cpp
    messagequeue.cpp
    messagerouter.cpp
    messages.cpp
    messagesender.cpp
    object.cpp
    objectmeta.cpp
    parameter.cpp
    parametermanager.cpp
    paramvector.cpp
    port.cpp
    porttracker.cpp
    shm.cpp
    shm_obj_ref.cpp
    shm_reference.cpp
    shmname.cpp
    statetracker.cpp
    tcpmessage.cpp
    vector.cpp)

# but just one file including all the others for faster compilation
set(core_SOURCES
    ${core_SOURCES}
    celltree.cpp
    coords.cpp
    coordswradius.cpp
    database.cpp
    empty.cpp
    grid.cpp
    indexed.cpp
    lines.cpp
    ngons.cpp
    normals.cpp
    placeholder.cpp
    points.cpp
    polygons.cpp
    rectilineargrid.cpp
    spheres.cpp
    structuredgrid.cpp
    structuredgridbase.cpp
    texture1d.cpp
    tubes.cpp
    uniformgrid.cpp
    unstr.cpp
    vec.cpp
    vertexownerlist.cpp)

set(core_HEADERS
    archive_loader.h
    archive_saver.h
    archives.h
    archives_config.h
    availablemodule.h
    cellalgorithm.h
    celltree.h
    celltree_impl.h
    celltreenode.h
    celltypes.h
    coords.h
    coords_impl.h
    coordswradius.h
    coordswradius_impl.h
    database.h
    database_impl.h
    dimensions.h
    empty.h
    empty_impl.h
    export.h
    filequery.h
    findobjectreferenceoarchive.h
    geometry.h
    grid.h
    index.h
    indexed.h
    indexed_impl.h
    lines.h
    lines_impl.h
    message.h
    messagepayload.h
    messagepayloadtemplates.h
    messagequeue.h
    messagerouter.h
    messages.h
    messagesender.h
    ngons.h
    ngons_impl.h
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
    placeholder.h
    placeholder_impl.h
    points.h
    points_impl.h
    polygons.h
    polygons_impl.h
    port.h
    porttracker.h
    quads.h
    rectilineargrid.h
    rectilineargrid_impl.h
    scalar.h
    scalars.h
    serialize.h
    shm.h
    shm_array.h
    shm_impl.h
    shm_obj_ref.h
    shm_obj_ref_impl.h
    shm_reference.h
    shm_reference_impl.h
    shmdata.h
    shmname.h
    shmvector.h
    spheres.h
    spheres_impl.h
    statetracker.h
    structuredgrid.h
    structuredgrid_impl.h
    structuredgridbase.h
    structuredgridbase_impl.h
    tcpmessage.h
    texture1d.h
    texture1d_impl.h
    triangles.h
    tubes.h
    tubes_impl.h
    uniformgrid.h
    uniformgrid_impl.h
    unstr.h
    unstr_impl.h
    uuid.h
    vec.h
    vec_impl.h
    vec_template.h
    vector.h
    vertexownerlist.h
    vertexownerlist_impl.h)
if(MSVC)
    set_source_files_properties(allobjects.cpp PROPERTIES COMPILE_FLAGS /bigobj)
    set_source_files_properties(cellalgorithm.cpp PROPERTIES COMPILE_FLAGS /bigobj)
    set_source_files_properties(unstr.cpp PROPERTIES COMPILE_FLAGS /bigobj)
    set_source_files_properties(vec.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

vistle_add_library(vistle_core ${VISTLE_LIB_TYPE} ${core_SOURCES} ${core_HEADERS})
vistle_export_library(vistle_core ${VISTLE_LIB_TYPE} ${core_SOURCES} ${core_HEADERS})
#vistle_install_headers(core ${core_HEADERS})

if(UNIX AND NOT APPLE)
    target_link_libraries(vistle_core PUBLIC -lrt)
endif()
if(WIN32)
    target_link_libraries(vistle_core PUBLIC bcrypt.lib)
endif()

vistle_target_link_libraries(
    vistle_core
    PRIVATE
    Threads::Threads
    PRIVATE
    vistle_util
    PUBLIC
    Boost::date_time
    PUBLIC
    Boost::serialization
    PUBLIC
    Boost::system)

if(ZFP_FOUND)

    target_compile_definitions(vistle_core PRIVATE HAVE_ZFP)
    target_include_directories(vistle_core SYSTEM PRIVATE ${ZFP_INCLUDE_DIRS})
    vistle_target_link_libraries(vistle_core PRIVATE ${ZFP_LIBRARIES})
endif()

if(ZSTD_FOUND)
    target_compile_definitions(vistle_core PRIVATE HAVE_ZSTD)
    target_include_directories(vistle_core SYSTEM PRIVATE ${ZSTD_INCLUDE_DIRS})
    vistle_target_link_libraries(vistle_core PRIVATE ${ZSTD_LIBRARIES})
endif()

if(LZ4_FOUND)
    target_compile_definitions(vistle_core PRIVATE HAVE_LZ4)
    target_include_directories(vistle_core SYSTEM PRIVATE ${LZ4_INCLUDE_DIRS})
    vistle_target_link_libraries(vistle_core PRIVATE ${LZ4_LIBRARIES})
endif()

if(SNAPPY_FOUND)
    target_compile_definitions(vistle_core PRIVATE HAVE_SNAPPY)
    target_include_directories(vistle_core SYSTEM PRIVATE ${SNAPPY_INCLUDE_DIRS})
    vistle_target_link_libraries(vistle_core PRIVATE ${SNAPPY_LIBRARIES})
endif()

target_include_directories(vistle_core SYSTEM PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdparty>)
target_include_directories(vistle_core SYSTEM PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdparty/yas/include>)
if(VISTLE_INSTALL_3RDPARTY)
    target_include_directories(vistle_core SYSTEM PUBLIC $<INSTALL_INTERFACE:include/vistle/3rdparty>)
endif()
