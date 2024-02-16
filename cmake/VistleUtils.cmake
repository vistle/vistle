macro(generate_network_snapshot targetname network_file)
    add_custom_command(
        #create a snapshot of the pipeline
        OUTPUT ${CMAKE_CURRENT_LIST_DIR}/${network_file}_workflow.png
        COMMAND vistle --snapshot ${CMAKE_CURRENT_LIST_DIR}/${network_file}_workflow.png ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${VISTLE_UMENTATION_WORKFLOW}.vsl targetname
        COMMENT "Generating network snapshot for " ${network_file}.vsl)

    # add_custom_target(${targetname}_${network_file}_workflow DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${network_file}_workflow.png)
    add_custom_target(${targetname}_${network_file}_workflow DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}_workflow.png)
    add_dependencies(${targetname}_doc ${targetname}_${network_file}_workflow)
endmacro()

option(VISTLE_DOC_WORKFLOW "Generate workflow snapshots (GUI + COVER) for documentation" ON)
macro(generate_snapshots_and_workflow targetname network_file output_src_dir)
    set(SNAPSHOT_ARGS "-name ${network_file} -o ${output_src_dir} -sdir ${CMAKE_CURRENT_LIST_DIR}")
    if(VISTLE_DOC_WORKFLOW)
        set(SNAPSHOT_ARGS "${SNAPSHOT_ARGS} -gui")
    endif()
    message("SNAPSHOT_ARGS: " ${SNAPSHOT_ARGS})
    #if we have a viewpoint file we can generate a result image, only first viewpoint is considered, only first cover is considered
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_LIST_DIR}/${network_file}_result.png
            COMMAND ${CMAKE_COMMAND} -E env COCONFIG=${PROJECT_SOURCE_DIR}/doc/config.vistle.doc.xml SNAPSHOT_ARGS=${SNAPSHOT_ARGS} vistle
                    ${PROJECT_SOURCE_DIR}/doc/resultSnapShot.py
            DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp ${targetname}
                    ${PROJECT_SOURCE_DIR}/doc/resultSnapShot.py
            COMMENT "Generating result snapshot for " ${network_file}.vsl)
        add_custom_target(${targetname}_${network_file}_result DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}_result.png)
        add_dependencies(${targetname}_doc ${targetname}_${network_file}_result)
    else()
        message(
            WARNING "can not generate snapshots for "
                    ${targetname}
                    " "
                    ${network_file}
                    ": missing viewpoint file, make sure a viewpoint file named \""
                    ${network_file}
                    ".vwp\" exists!")
    endif()
endmacro()
