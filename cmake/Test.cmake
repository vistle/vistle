macro(add_image_hash targetname)
    add_custom_target(${targetname}_hash)

    if(MODULE_CAN_CREATE_HASH)
        add_dependencies(vistle_ref_hash ${targetname}_hash)
    else()
        message("Skipping ${targetname}_hash")
    endif()

    file(
        GLOB WORKFLOWS
        LIST_DIRECTORIES FALSE
        ${CMAKE_CURRENT_SOURCE_DIR}/*.vsl)
    foreach(file ${WORKFLOWS})
        get_filename_component(workflow ${file} NAME_WLE)
        # message("Workflow: ${targetname} ${workflow} ${file}")
        generate_cover_snapshot(${targetname} ${workflow} ${CMAKE_CURRENT_SOURCE_DIR})
        create_image_hash(${targetname} ${workflow})
        # TODO: uncomment later
        #generate_snapshots_and_workflow(${targetname} ${workflow} ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach()
endmacro()
