macro(generate_cover_snapshot targetname network_file add_workflow_snapshot output_dir target_dependant)
    set(SNAPSHOT_ARGS "-name ${network_file} -o ${output_dir} -sdir ${CMAKE_CURRENT_LIST_DIR}")
    set(OUTPUT_TMP ${output_dir}/${network_file}_result.png)
    if(${add_workflow_snapshot})
        set(SNAPSHOT_ARGS "${SNAPSHOT_ARGS} -gui")
        set(OUTPUT_TMP ${output_dir}/${network_file}_workflow.png)
    endif()

    #if we have a viewpoint file we can generate a result image, only first viewpoint is considered, only first cover is considered
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp)
        add_custom_command(
            OUTPUT ${OUTPUT_TMP}
            COMMAND ${CMAKE_COMMAND} -E env COCONFIG=${PROJECT_SOURCE_DIR}/doc/config.vistle.doc.xml SNAPSHOT_ARGS=${SNAPSHOT_ARGS} vistle
                    ${PROJECT_SOURCE_DIR}/doc/resultSnapShot.py
            DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp ${targetname}
                    ${PROJECT_SOURCE_DIR}/doc/resultSnapShot.py
            COMMENT "Generating result snapshot for " ${network_file}.vsl)
        
        set(custom_targetname ${target_dependant}_${network_file})
        add_custom_target(${custom_targetname} DEPENDS ${OUTPUT_TMP})
        add_dependencies(${target_dependant} ${custom_targetname})
        
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

macro(create_image_hash targetname network_file)
    file(RELATIVE_PATH mod_path ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    message("mod_path: ${mod_path}")
    # set(HASH_ARGS -name ${targetname} -sdir ${CMAKE_CURRENT_LIST_DIR}/${network_file}_result.png -jdir
    set(HASH_ARGS -name ${mod_path}/${network_file}.vsl -sdir ${CMAKE_CURRENT_LIST_DIR}/${network_file}_result.png -jdir
                  ${PROJECT_SOURCE_DIR}/test/moduleTest/utils/refImageHash.json)

    # #if we have a viewpoint file we can generate a result image, only first viewpoint is considered, only first cover is considered
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp)
        message("Creating image hash for " ${network_file}_result.png)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_LIST_DIR}/${network_file}_hash_out
            COMMAND ${Python_EXECUTABLE} ${PROJECT_SOURCE_DIR}/test/moduleTest/utils/createImageHash.py ${HASH_ARGS}
            DEPENDS ${PROJECT_SOURCE_DIR}/test/moduleTest/utils/createImageHash.py ${targetname} ${CMAKE_CURRENT_LIST_DIR}/${network_file}_result.png
            COMMENT "Create image hash for: " ${network_file}_result.png)
        add_custom_target(${targetname}_${network_file}_resultHash DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}_hash_out)
        add_dependencies(${targetname}_hash ${targetname}_${network_file}_resultHash)
    else()
        message(WARNING "cannot create image hash for " ${targetname})
    endif()
endmacro()
