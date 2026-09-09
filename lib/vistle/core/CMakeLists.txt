set(core_SOURCES
    message/colormap.cpp
    message/setname.cpp
    allobjects.cpp # just one file including all the others for faster compilation
    archive_loader.cpp
    archive_saver.cpp
    archives_compress_bigwhoop.cpp
    archives_compress_sz3.cpp
    archives_compress.cpp
    archives.cpp
    availablemodule.cpp
    cellalgorithm.cpp
    envelope.cpp
    filequery.cpp
    geometry.cpp
    message.cpp
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
    shm_array.cpp
    shmenvelope.cpp
    shm_obj_ref.cpp
    shm_reference.cpp
    shm.cpp
    shmname.cpp
    statetracker.cpp
    tcpmessage.cpp
    vector.cpp)

# but just one file including all the others for faster compilation
set(core_SOURCES
    ${core_SOURCES}
    celltree.cpp
    celltreenode.cpp
    coords.cpp
    database.cpp
    empty.cpp
    grid.cpp
    indexed.cpp
    layergrid.cpp
    lines.cpp
    ngons.cpp
    normals.cpp
    placeholder.cpp
    points.cpp
    polygons.cpp
    rectilineargrid.cpp
    scalars.cpp
    structuredgrid.cpp
    structuredgridbase.cpp
    uniformgrid.cpp
    unstr.cpp
    uuid.cpp
    vec.cpp
    vertexownerlist.cpp)

set(core_HEADERS
    archive_loader.h
    archive_saver.h
    archives_compress_bigwhoop.h
    archives_compress_sz3.h
    archives_compress.h
    archives_config.h
    archives_impl.h
    archives.h
    availablemodule.h
    cellalgorithm.h
    celltree_impl.h
    celltree.h
    celltreenode_decl.h
    celltreenode.h
    celltypes.h
    coords_impl.h
    coords.h
    database_impl.h
    database.h
    dimensions.h
    empty_impl.h
    empty.h
    envelope.h
    export.h
    filequery.h
    geometry.h
    grid.h
    index.h
    indexed_impl.h
    indexed.h
    layergrid_impl.h
    layergrid.h
    lines_impl.h
    lines.h
    message.h
    message/colormap.h
    message/setname.h
    messagepayload.h
    messagepayloadtemplates.h
    messagequeue.h
    messagerouter.h
    messages.h
    messagesender.h
    ngons_impl.h
    ngons.h
    normals_impl.h
    normals.h
    object_impl.h
    object.h
    objectmeta_impl.h
    objectmeta.h
    parameter.h
    parametermanager_impl.h
    parametermanager.h
    paramvector_impl.h
    paramvector.h
    placeholder_impl.h
    placeholder.h
    points_impl.h
    points.h
    polygons_impl.h
    polygons.h
    port.h
    porttracker.h
    quads.h
    rectilineargrid_impl.h
    rectilineargrid.h
    rgba.h
    scalar.h
    scalars.h
    serialize.h
    shm_array_impl.h
    shm_array.h
    shm_config.h
    shmenvelope.h
    shm_impl.h
    shm_obj_ref_impl.h
    shm_obj_ref.h
    shm_reference_impl.h
    shm_reference.h
    shm.h
    shmdata.h
    shmname.h
    shmvector.h
    statetracker.h
    structuredgrid_impl.h
    structuredgrid.h
    structuredgridbase_impl.h
    structuredgridbase.h
    tcpmessage.h
    triangles.h
    uniformgrid_impl.h
    uniformgrid.h
    unstr_geo.h
    unstr_impl.h
    unstr.h
    uuid.h
    vec_impl.h
    vec_template.h
    vec.h
    vector.h
    vectortypes.h
    vertexownerlist_impl.h
    vertexownerlist.h)

if(MSVC)
    set_source_files_properties(allobjects.cpp PROPERTIES COMPILE_FLAGS /bigobj)
    set_source_files_properties(cellalgorithm.cpp PROPERTIES COMPILE_FLAGS /bigobj)
    set_source_files_properties(unstr.cpp PROPERTIES COMPILE_FLAGS /bigobj)
    set_source_files_properties(vec.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

vistle_add_library(vistle_core EXPORT ${VISTLE_LIB_TYPE} ${core_SOURCES} ${core_HEADERS})

target_link_libraries(vistle_core PUBLIC Eigen3::Eigen)
target_link_libraries(vistle_core PUBLIC vistle_yas)
if(UNIX AND NOT APPLE)
    target_link_libraries(vistle_core PUBLIC -lrt)
endif()
if(WIN32)
    target_link_libraries(vistle_core PUBLIC bcrypt.lib)
endif()
target_compile_definitions(vistle_core PUBLIC BOOST_MPL_LIMIT_VECTOR_SIZE=30)
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
    Boost::system
    PUBLIC
    vistle::viskores_cont)
target_link_libraries(vistle_core PRIVATE vistle_config)

target_include_directories(vistle_core SYSTEM PRIVATE ${ZFP_INCLUDE_DIRS})
vistle_target_link_libraries(vistle_core PRIVATE ${ZFP_LIBRARIES})

if(SZ3_FOUND)
    target_compile_definitions(vistle_core PRIVATE HAVE_SZ3)
    target_include_directories(vistle_core SYSTEM PRIVATE ${SZ3_INCLUDE_DIRS})
    vistle_target_link_libraries(vistle_core PRIVATE ${SZ3_LIBRARIES})
endif()

target_include_directories(vistle_core SYSTEM PRIVATE ${BIGWHOOP_INCLUDE_DIRS})
vistle_target_link_libraries(vistle_core PRIVATE bwclib)

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
