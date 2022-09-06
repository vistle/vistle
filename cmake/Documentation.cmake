macro(add_module_doc_target targetname)

    set(VISTLE_DOCUMENTATION_WORKFLOW ${PROJECT_SOURCE_DIR}/doc/generateModuleInfo.vsl)
    set(DOC_COMMAND ${CMAKE_COMMAND} -E env 
        VISTLE_DOCUMENTATION_TARGET=${targetname}
        VISTLE_DOCUMENTATION_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        VISTLE_DOCLUMENTATION_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}
            vistle --batch ${VISTLE_DOCUMENTATION_WORKFLOW})

    set(OUTPUT_FILE ${PROJECT_SOURCE_DIR}/doc/GenModInfo/moduleDescriptions/${targetname}.md)
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
    add_custom_target(
        ${targetname}_doc
        DEPENDS ${OUTPUT_FILE})
    if (VISTLE_DOC_SKIP)
        message("Skipping ${targetname}_doc: ${VISTLE_DOC_SKIP}")
        add_dependencies(vistle_doc_skip ${targetname}_doc)
    else()
        add_dependencies(vistle_doc ${targetname}_doc)
    endif()
endmacro()

macro(generate_network_snapshot targetname network_file)
    add_custom_command( #create a snapshot of the pipeline
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${network_file}_workflow.png
        COMMAND vistle --snapshot ${CMAKE_CURRENT_BINARY_DIR}/${network_file}_workflow.png ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl
        DEPENDS  ${CMAKE_CURRENT_LIST_DIR}/${VISTLE_DOCUMENTATION_WORKFLOW}.vsl targetname
        COMMENT "Generating network snapshot for " ${network_file}.vsl)

    add_custom_target(
        ${targetname}_${VISTLE_DOCUMENTATION_WORKFLOW}_workflow
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${VISTLE_DOCUMENTATION_WORKFLOW}_workflow.png)
    add_dependencies(${targetname}_doc ${targetname}_${VISTLE_DOCUMENTATION_WORKFLOW}_workflow)
endmacro()

macro(generate_snapshots targetname network_file)
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp) #if we have a viewpoint file we can generate an result image, only first viewpoint is considered, only first cover is considered
        add_custom_command(
            OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${network_file}_result.png ${CMAKE_CURRENT_BINARY_DIR}/${network_file}_workflow.png
            COMMAND ${CMAKE_COMMAND} -E env 
                COCONFIG=${PROJECT_SOURCE_DIR}/doc/config.vistle.doc.xml 
                VISTLE_DOC_IMAGE_NAME=${network_file}
                VISTLE_DOC_SOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}
                VISTLE_DOC_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
                     vistle ${PROJECT_SOURCE_DIR}/doc/resultSnapShot.py
            DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp ${targetname}
            COMMENT "Generating network and result snapshot for " ${network_file}.vsl)
        add_custom_target(
            ${targetname}_${network_file}_result
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${network_file}_result.png ${CMAKE_CURRENT_BINARY_DIR}/${network_file}_workflow.png)
        add_dependencies(${targetname}_doc ${targetname}_${network_file}_result)
    else()
        message(WARNING "can not generate snapshots for " ${targetname} " " ${network_file} ": missing viewpoint file, make sure a viewpoint file named \"" ${network_file} ".vwp\" exists!")
    endif()
endmacro()
