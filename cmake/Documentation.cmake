set(ALL_VISTLE_MODULES_CATEGORY
    ""
    CACHE INTERNAL "")
set(ADDITIONAL_DOCUMENTATION_SOURCES
    ""
    CACHE INTERNAL "Markdown files that belong to the source tree and are used in the documentation")

macro(add_documentation_source)
    foreach(source ${ARGN})
        set(ADDITIONAL_DOCUMENTATION_SOURCES
            ${ADDITIONAL_DOCUMENTATION_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/${source}
            CACHE INTERNAL "Markdown files that belong to the source tree and are used in the documentation")
    endforeach()
endmacro()

set(IGNR_MODS "COVER_plugin")

macro(configure_documentation_detail INPUT_FILE OUTPUT_FILE TARGET)

    list(APPEND ${TARGET} ${OUTPUT_FILE})
    if(${DOCUMENTATION_FILE} MATCHES ".*\\.md")

        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${CONFIGURE_COMMAND} ${INPUT_FILE} ${OUTPUT_FILE}
            DEPENDS ${INPUT_FILE} vistle_module_doc ${CMAKE_SOURCE_DIR}/doc/tools/insertModuleLinks.py
            COMMENT "Configuring file: ${OUTPUT_FILE}")
    else()
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy ${INPUT_FILE} ${OUTPUT_FILE}
            DEPENDS ${INPUT_FILE}
            COMMENT "Copying file: ${OUTPUT_FILE}")
    endif()
endmacro()

function(configure_documentation)

    # Find all files in the ToInstall directory recursively
    message("CMAKE_COMMAND: " ${CMAKE_COMMAND})
    set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/doc/source)
    file(
        GLOB_RECURSE DOCUMENTATION_FILES
        RELATIVE ${SOURCE_DIR}
        "${SOURCE_DIR}/*")
    # List to hold all the output files
    set(CONFIGURED_FILES "")
    # Configure each file and add to the list of output files
    set(CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env ALL_VISTLE_MODULES="${ALL_MODULES}" ALL_VISTLE_MODULES_CATEGORY="${ALL_VISTLE_MODULES_CATEGORY}"
                          ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/doc/tools/insertModuleLinks.py ${PROJECT_SOURCE_DIR} ${VISTLE_DOCUMENTATION_SOURCE_DIR})
    foreach(DOCUMENTATION_FILE ${DOCUMENTATION_FILES})
        set(INPUT_FILE ${SOURCE_DIR}/${DOCUMENTATION_FILE})
        set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/${DOCUMENTATION_FILE})
        configure_documentation_detail(${INPUT_FILE} ${OUTPUT_FILE} CONFIGURED_FILES)
    endforeach()
    add_custom_target(configure_documentation_files DEPENDS ${CONFIGURED_FILES})
    add_dependencies(vistle_doc configure_documentation_files)
    #configure links to other modules in the module generated markdowns
    set(CONFIGURED_MODULE_FILES)
    #get all directories under ${CMAKE_BINARY_DIR}/docs/source/module/
    file(
        GLOB ALL_VISTLE_MODULES_CATEGORIES
        RELATIVE ${CMAKE_BINARY_DIR}/docs/source/module
        ${CMAKE_BINARY_DIR}/docs/source/module/*)

    foreach(MODULE ${ALL_MODULES})
        #ignore modules in the IGNR_MODS list
        set(ignore FALSE)
        foreach(elem ${IGNR_MODS})
            if(${MODULE} STREQUAL ${elem})
                set(ignore TRUE)
                break()
            endif()
        endforeach()
        if(${ignore})
            continue()
        endif()
        #get the category of the module
        list(FIND ALL_MODULES ${MODULE} CATEGORY_INDEX)
        list(GET ALL_VISTLE_MODULES_CATEGORY ${CATEGORY_INDEX} CATEGORY)
        #relink and install the module markdowns
        set(INPUT_FILE ${CMAKE_BINARY_DIR}/docs/source/module/${CATEGORY}/${MODULE}.md)
        set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/${CATEGORY}/${MODULE}/${MODULE}.md)
        list(APPEND CONFIGURED_MODULE_FILES ${OUTPUT_FILE})
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${CONFIGURE_COMMAND} ${INPUT_FILE} ${OUTPUT_FILE}
            DEPENDS ${INPUT_FILE} vistle_module_doc ${CMAKE_SOURCE_DIR}/doc/tools/insertModuleLinks.py
            COMMENT "Configuring module file: ${OUTPUT_FILE}")
    endforeach()

    add_custom_target(configure_module_files DEPENDS ${CONFIGURED_MODULE_FILES})
    add_dependencies(vistle_doc configure_module_files)

    # Custom command to copy specified documentation files from source tree
    set(CONFIGURED_FILES_FROM_SOURCE_TREE "")
    foreach(README_FILE ${ADDITIONAL_DOCUMENTATION_SOURCES})
        file(RELATIVE_PATH RELATIVE_PATH ${CMAKE_CURRENT_LIST_DIR} ${README_FILE})
        set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_DIR}/docs/readme/${RELATIVE_PATH})
        configure_documentation_detail(${README_FILE} ${OUTPUT_FILE} CONFIGURED_FILES_FROM_SOURCE_TREE)
    endforeach()
    add_custom_target(copy_readme_files DEPENDS ${CONFIGURED_FILES_FROM_SOURCE_TREE})
    add_dependencies(vistle_doc copy_readme_files)

endfunction()

set(VISTLE_DOCUMENTATION_DIR
    "${PROJECT_BINARY_DIR}/documentation"
    CACHE PATH "Path where the documentation will be built")
vistle_find_package(Sphinx)
if(SPHINX_EXECUTABLE)

    add_custom_target(vistle_module_doc)
    add_custom_target(vistle_doc)
    add_dependencies(vistle_doc vistle_module_doc)

    set(READTHEDOCS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/doc/readthedocs)
    set(VISTLE_DOCUMENTATION_SOURCE_DIR ${VISTLE_DOCUMENTATION_DIR}/docs/source)

    #copy the readthedocs configuration scripts
    add_custom_command(
        TARGET vistle_doc
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/clear.py ${VISTLE_DOCUMENTATION_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/conf.py ${VISTLE_DOCUMENTATION_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/mdlink.py ${VISTLE_DOCUMENTATION_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/html_image_processor.py ${VISTLE_DOCUMENTATION_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/requirements.txt ${VISTLE_DOCUMENTATION_DIR}/docs
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/.readthedocs.yaml ${VISTLE_DOCUMENTATION_DIR})

    add_custom_command(TARGET vistle_doc COMMAND ${CMAKE_COMMAND} -E make_directory ${VISTLE_DOCUMENTATION_DIR}/docs/build)
    add_custom_command(
        TARGET vistle_doc
        COMMAND ${SPHINX_EXECUTABLE} -M html source build
        WORKING_DIRECTORY ${VISTLE_DOCUMENTATION_DIR}/docs
        COMMENT "Building readTheDocs documentation" DEPENDS vistle_module_doc)

    set(VISTLE_BUILD_DOC TRUE)
else(SPHINX_EXECUTABLE)
    message("Sphinx not found, documentation can not be built")
    set(VISTLE_BUILD_DOC FALSE)
    return()

endif(SPHINX_EXECUTABLE)

macro(append_module_category targetname)

    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
    get_filename_component(CATEGORY ${PARENT_DIR} NAME)
    set(ALL_VISTLE_MODULES_CATEGORY
        ${ALL_VISTLE_MODULES_CATEGORY} ${CATEGORY}
        CACHE INTERNAL "")

endmacro()

macro(add_module_doc_target targetname)

    set(VISTLE_DOCUMENTATION_WORKFLOW ${PROJECT_SOURCE_DIR}/doc/tools/generateModuleInfo.vsl)
    #insert module paramerts in markdown files
    set(DOC_COMMAND
        ${CMAKE_COMMAND} -E env VISTLE_DOCUMENTATION_TARGET=${targetname} VISTLE_BINARY_DIR=${CMAKE_BINARY_DIR}
        VISTLE_MODULE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} VISTLE_DOCUMENTATION_CATEGORY=${CATEGORY}
        VISTLE_DOCUMENTATION_SOURCE_DIR=${VISTLE_DOCUMENTATION_SOURCE_DIR} vistle --batch ${VISTLE_DOCUMENTATION_WORKFLOW})

    set(OUTPUT_FILE ${CMAKE_BINARY_DIR}/docs/source/module/${CATEGORY}/${targetname}.md)
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
                ${DOCUMENTATION_DEPENDENCIES} #custom dependencies set by the calling module
        COMMENT "Generating documentation for ${targetname}")
    add_custom_target(${targetname}_doc DEPENDS ${OUTPUT_FILE})

    add_dependencies(vistle_module_doc ${targetname}_doc)

    file(
        GLOB WORKFLOWS
        LIST_DIRECTORIES FALSE
        ${CMAKE_CURRENT_SOURCE_DIR}/*.vsl)
    foreach(file ${WORKFLOWS})
        get_filename_component(workflow ${file} NAME_WLE)
        message("Workflow: ${targetname} ${workflow} ${file}")

        set(output_dir ${VISTLE_DOCUMENTATION_DIR}/docs/source/module/${CATEGORY}/${targetname})
        #generating the result and workflow snapshots from different vistle instances
        #prevents vistle from asking to save the workflow
        generate_result_snapshot(${targetname} ${workflow} ${output_dir})
        generate_network_snapshot_and_quit(${targetname} ${workflow} ${output_dir})
        # add_dependencies(${targetname}_${network_file}_result ${targetname}_${network_file}_workflow)
    endforeach()
endmacro()

macro(generate_snapshot_base targetname network_file output_dir workflow result)

    if((${result} AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp) OR ${workflow}
    )#if we have a viewpoint file we can generate an result image, only first viewpoint is considered, only first cover is considered
        set(batch "")
        if(NOT ${workflow})
            set(batch "--batch")
        endif()
        set(VISTLE_DOC_ARGS)
        set(output_file)
        set(custom_target ${targetname}_${network_file})
        if(${result})
            set(VISTLE_DOC_ARGS ${VISTLE_DOC_ARGS} result)
            set(output_file ${output_file} ${output_dir}/${network_file}_result.png)
            set(custom_target ${custom_target}_result)
        endif()
        if(${workflow})
            set(VISTLE_DOC_ARGS ${VISTLE_DOC_ARGS} workflow)
            set(output_file ${output_file} ${output_dir}/${network_file}_workflow.png)
            set(custom_target ${custom_target}_workflow)
        endif()
        message("add_custom_command snapshot for " ${network_file} " " ${custom_target})
        message("output_file: " ${output_file})
        add_custom_command(
            OUTPUT ${output_file}
            COMMAND
                ${CMAKE_COMMAND} -E env COCONFIG=${PROJECT_SOURCE_DIR}/doc/config.vistle.doc.xml VISTLE_DOC_IMAGE_NAME=${network_file}
                VISTLE_DOC_SOURCE_DIR=${CMAKE_CURRENT_LIST_DIR} VISTLE_DOC_TARGET_DIR=${output_dir} VISTLE_DOC_ARGS=${VISTLE_DOC_ARGS} vistle -vvvv ${batch}
                ${PROJECT_SOURCE_DIR}/doc/tools/snapShot.py
            DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp ${targetname}
                    ${PROJECT_SOURCE_DIR}/doc/tools/snapShot.py
            COMMENT "Generating network and result snapshot for ${network_file}.vsl")
        add_custom_target(${custom_target} DEPENDS ${output_file})
        add_dependencies(${targetname}_doc ${custom_target})
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

#generate snapshots for a workflow and result from a single vistle instance
macro(generate_result_and_workflow_snapshot targetname network_file output_dir)
    generate_snapshot_base(${targetname} ${network_file} ${output_dir} TRUE TRUE)
endmacro()

macro(generate_result_snapshot targetname network_file output_dir)
    generate_snapshot_base(${targetname} ${network_file} ${output_dir} FALSE TRUE)
endmacro()

macro(generate_network_snapshot_and_quit targetname network_file output_dir)
    generate_snapshot_base(${targetname} ${network_file} ${output_dir} TRUE FALSE)
endmacro()

#not used for now, does not require special script but leaves vistle running
macro(generate_network_snapshot targetname network_file output_dir)
    add_custom_command(
        #create a snapshot of the pipeline
        OUTPUT ${output_dir}/${network_file}_workflow.png
        COMMAND vistle --snapshot ${output_dir}/${network_file}_workflow.png ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl ${targetname}
        COMMENT "Generating network snapshot for ${network_file}.vsl")

    add_custom_target(${targetname}_${network_file}_workflow DEPENDS ${output_dir}/${network_file}_workflow.png)
    add_dependencies(${targetname}_doc ${targetname}_${network_file}_workflow)
endmacro()
