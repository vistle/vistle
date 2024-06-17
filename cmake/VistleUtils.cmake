# Generates a screenshot of the COVER output of one .vsl file in a module's build dir, provided a .vwp viewpoint
# file lies in the directory. Optionally, a screenshot of the GUI pipeline is created as well.
macro(generate_cover_snapshot targetname network_file add_workflow_snapshot output_dir target_dependant)
    set(SNAPSHOT_ARGS "-name ${network_file} -o ${output_dir} -sdir ${CMAKE_CURRENT_LIST_DIR}")
    set(custom_command_output ${output_dir}/${network_file}_result.png)

    if(${add_workflow_snapshot})
        set(SNAPSHOT_ARGS "${SNAPSHOT_ARGS} -gui")
        # TODO: add_custom_command can only be called ONCE for the same output file. Since (1) the macros add_image_hash and add_module_doc_target
        #       both call generate_cover_snapshot and (2) resultSnapShot.py in both cases creates the same two .png files (i.e., ${network_file}_result.png
        #       and ${network_file}_workflow.png}, we pass ${network_file}_workflow.png to OUTPUT (instead of ${network_file}_result.png) if the GUI screen-
        #       shots are created.
        #       While this works as workaround for our two different targets, it does not work with 3 or more targets, so if generate_cover_snapshot is to
        #       be called by another CMake macro, this still needs to get resolved. E.g., by letting the user choose the name of the output images result-
        #       SnapShot.py creates (however, first we need to determine all places in the code which assume that the screenshots are saved under
        #       ${network_file}_result.png and ${network_file}_workflow.png and adjust them).
        set(custom_command_output ${output_dir}/${network_file}_workflow.png)
    endif()

    # if we have a viewpoint file we can generate a result image, only first viewpoint is considered, only first cover is considered
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp)
        add_custom_command(
            OUTPUT ${custom_command_output}
            COMMAND ${CMAKE_COMMAND} -E env COCONFIG=${PROJECT_SOURCE_DIR}/doc/config.vistle.doc.xml SNAPSHOT_ARGS=${SNAPSHOT_ARGS} vistle
                    ${PROJECT_SOURCE_DIR}/doc/resultSnapShot.py
            DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp ${targetname}
                    ${PROJECT_SOURCE_DIR}/doc/resultSnapShot.py
            COMMENT "Generating result snapshot for " ${network_file}.vsl)

        # to execute the custom command, it must depend on a target (in this case the one passed as parameter)
        set(custom_targetname screenshot_${target_dependant}_${network_file})
        add_custom_target(${custom_targetname} DEPENDS ${custom_command_output})
        add_dependencies(${target_dependant} ${custom_targetname})

    else()
        message(
            WARNING "cannot generate snapshots for "
                    ${targetname}
                    " "
                    ${network_file}
                    ": missing viewpoint file, make sure a viewpoint file named \""
                    ${network_file}
                    ".vwp\" exists!")
    endif()
endmacro()
