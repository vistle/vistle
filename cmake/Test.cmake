macro(add_image_hash targetname)
    add_custom_target(${targetname}_hash)

    add_dependencies(vistle_ref_hash ${targetname}_hash)

    # loop over all .vsl files inside the folder of the current module
    file(
        GLOB WORKFLOWS
        LIST_DIRECTORIES FALSE
        ${CMAKE_CURRENT_SOURCE_DIR}/*.vsl)
        
    foreach(file ${WORKFLOWS})
        # remove directory and extension from .vsl file
        get_filename_component(workflow ${file} NAME_WLE)
        
        generate_cover_snapshot(${targetname} ${workflow} FALSE ${CMAKE_CURRENT_SOURCE_DIR} ${targetname}_hash)
        create_image_hash(${targetname} ${workflow})
    endforeach()
endmacro()
