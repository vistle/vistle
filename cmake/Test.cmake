# generates a screenshot of the COVER output of the module's .vsl file, calculates the 
# corresponding image hash and adds it to a .json file
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

# creates an image hash for ${network_file}_result.png and writes it to refImageHash.json
macro(create_image_hash targetname network_file)
    file(RELATIVE_PATH mod_path ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    set(HASH_ARGS -name ${mod_path}/${network_file}.vsl -sdir ${CMAKE_CURRENT_LIST_DIR}/${network_file}_result.png -jdir
                  ${PROJECT_SOURCE_DIR}/test/moduleTest/utils/refImageHash.json)
    set(custom_command_output ${CMAKE_CURRENT_LIST_DIR}/${network_file}_hash_out)

    # if we have a viewpoint file we can generate a result image, only first viewpoint is considered, only first cover is considered
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp)
        add_custom_command(
            OUTPUT ${custom_command_output}
            COMMAND ${Python_EXECUTABLE} ${PROJECT_SOURCE_DIR}/test/moduleTest/utils/createImageHash.py ${HASH_ARGS}
            DEPENDS ${PROJECT_SOURCE_DIR}/test/moduleTest/utils/createImageHash.py ${targetname} ${CMAKE_CURRENT_LIST_DIR}/${network_file}_result.png
            COMMENT "Create image hash for: " ${network_file}_result.png)
        
        set(custom_targetname image_hash_${target_dependant}_${network_file}) 
        add_custom_target(${custom_targetname} DEPENDS ${custom_command_output})
        add_dependencies(${targetname}_hash ${custom_targetname})
    else()
        message(
            WARNING "cannot create image hash for "
                    ${targetname}
                    " "
                    ${network_file}
                    ": missing viewpoint file, make sure a viewpoint file named \""
                    ${network_file}
                    ".vwp\" exists!")
    endif()
endmacro()
