macro(add_image_hash targetname)
    set(VISTLE_DOCUMENTATION_WORKFLOW ${PROJECT_SOURCE_DIR}/doc/generateModuleInfo.vsl)
    set(DOC_COMMAND ${CMAKE_COMMAND} -E env VISTLE_DOCUMENTATION_TARGET=${targetname} VISTLE_DOCUMENTATION_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                    VISTLE_DOCUMENTATION_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR} vistle --batch ${VISTLE_DOCUMENTATION_WORKFLOW})

    set(OUTPUT_FILE ${PROJECT_SOURCE_DIR}/docs/source/modules/${targetname}.md)
    set(INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${targetname}.md)
    if(NOT EXISTS ${INPUT_FILE})
        set(INPUT_FILE)
    endif()

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${DOC_COMMAND}
        DEPENDS #build if changes in:
                ${INPUT_FILE} #the custom documentation
                ${targetname} #the module's source code
                ${VISTLE_DOCUMENTATION_WORKFLOW} #the file that gets loaded by vistle to generate the documentation
                ${PROJECT_SOURCE_DIR}/doc/GenModInfo/genModInfo.py #dependency of VISTLE_DOCUMENTATION_WORKFLOW
                ${DOCUMENTATION_DEPENDENCIES} #custom dependencies set by the calling module
        COMMENT "Generating documentation for " ${targetname})
    add_custom_target(${targetname}_hash DEPENDS ${OUTPUT_FILE})
    
    if(MODULE_CAN_CREATE_HASH)
        add_dependencies(vistle_hash ${targetname}_hash)
    else()
        message("Skipping ${targetname}_hash")
        #add_dependencies(vistle_hash_skip ${targetname}_hash)
    endif()

    file(
        GLOB WORKFLOWS
        LIST_DIRECTORIES FALSE
        ${CMAKE_CURRENT_SOURCE_DIR}/*.vsl)
    foreach(file ${WORKFLOWS})
        get_filename_component(workflow ${file} NAME_WLE)
        message("Workflow: ${targetname} ${workflow} ${file}")
        generate_cover_snapshot(${targetname} ${workflow} ${CMAKE_CURRENT_SOURCE_DIR})
        # TODO: uncomment later
        #generate_snapshots_and_workflow(${targetname} ${workflow} ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach()
endmacro()
