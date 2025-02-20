set(ALL_VISTLE_MODULES_CATEGORY
    ""
    CACHE INTERNAL "")

function(copy_readme)

    # Find all README.md files recursively
    file(
        GLOB_RECURSE README_FILES
        RELATIVE ${CMAKE_SOURCE_DIR}
        "README.md" "*/README.md")
    set(README_OUTPUT_FILES)
    # Custom command to copy README.md files during the build process
    foreach(README_FILE ${README_FILES})
        get_filename_component(README_DIR ${README_FILE} DIRECTORY)
        set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_DIR}/docs/readme/${README_FILE})
        list(APPEND README_OUTPUT_FILES ${OUTPUT_FILE})
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            DEPENDS ${CMAKE_SOURCE_DIR}/${README_FILE} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/${README_FILE} ${OUTPUT_FILE}
            COMMENT "Copying ${README_FILE} to ${OUTPUT_FILE}")
    endforeach()

    # Custom target to copy all README.md files
    add_custom_target(copy_readme_files DEPENDS ${README_OUTPUT_FILES})
    add_dependencies(vistle_doc copy_readme_files)
endfunction()

function(configure_documentation)

    # Find all files in the ToInstall directory recursively

    set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/doc/source)
    file(
        GLOB_RECURSE DOCUMENTATION_FILES
        RELATIVE ${SOURCE_DIR}
        "${SOURCE_DIR}/*")
    # List to hold all the output files
    set(CONFIGURED_FILES)
    # Configure each file and add to the list of output files
    set(CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env ALL_VISTLE_MODULES="${ALL_MODULES}" ALL_VISTLE_MODULES_CATEGORY="${ALL_VISTLE_MODULES_CATEGORY}" python
                          ${CMAKE_SOURCE_DIR}/doc/tools/insertModuleLinks.py ${PROJECT_SOURCE_DIR} ${VISTLE_DOCUMENTATION_SOURCE_DIR})
    foreach(DOCUMENTATION_FILE ${DOCUMENTATION_FILES})
        set(INPUT_FILE ${SOURCE_DIR}/${DOCUMENTATION_FILE})
        set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/${DOCUMENTATION_FILE})
        list(APPEND CONFIGURED_FILES ${OUTPUT_FILE})

        if(${DOCUMENTATION_FILE} MATCHES ".*\\.md")

            add_custom_command(
                OUTPUT ${OUTPUT_FILE} POST_BUILD
                COMMAND ${CONFIGURE_COMMAND} ${INPUT_FILE} ${OUTPUT_FILE}
                DEPENDS ${INPUT_FILE} vistle_module_doc ${CMAKE_SOURCE_DIR}/doc/tools/insertModuleLinks.py
                COMMENT "Configuring file: ${OUTPUT_FILE}")
        else()
            add_custom_command(
                OUTPUT ${OUTPUT_FILE} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${INPUT_FILE} ${OUTPUT_FILE}
                DEPENDS ${INPUT_FILE})
        endif()

    endforeach()
    add_custom_target(configure_documentation_files DEPENDS ${CONFIGURED_FILES})
    add_dependencies(vistle_doc configure_documentation_files)
    #configure links to other modules in the module generated markdowns
    set(CONFIGURED_MODULE_FILES)
    #get all directories under ${CMAKE_BINARY_DIR}/docs/source/modules/
    file(
        GLOB ALL_VISTLE_MODULES_CATEGORIES
        RELATIVE ${CMAKE_BINARY_DIR}/docs/source/modules
        ${CMAKE_BINARY_DIR}/docs/source/modules/*)
    foreach(CATEGORY_DIR ${ALL_VISTLE_MODULES_CATEGORIES})
        file(
            GLOB_RECURSE MODULE_MARKDOWNS
            RELATIVE ${CMAKE_BINARY_DIR}/docs/source/modules/${CATEGORY_DIR}
            "${CMAKE_BINARY_DIR}/docs/source/modules/${CATEGORY_DIR}/*.md")
        message("modules in category: ${CATEGORY_DIR} ${MODULE_MARKDOWNS}")
        foreach(MODULE_MARKDOWN ${MODULE_MARKDOWNS})
            set(INPUT_FILE ${CMAKE_BINARY_DIR}/docs/source/modules/${CATEGORY_DIR}/${MODULE_MARKDOWN})
            set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/modules/${CATEGORY_DIR}/${MODULE_MARKDOWN})
            list(APPEND CONFIGURED_MODULE_FILES ${OUTPUT_FILE})
            add_custom_command(
                OUTPUT ${OUTPUT_FILE} POST_BUILD
                COMMAND ${CONFIGURE_COMMAND} ${INPUT_FILE} ${OUTPUT_FILE}
                DEPENDS ${INPUT_FILE} vistle_module_doc ${CMAKE_SOURCE_DIR}/doc/tools/insertModuleLinks.py
                COMMENT "Configuring module file: ${OUTPUT_FILE}")
        endforeach()

    endforeach()

    message("Configured documentation files: ${CONFIGURED_FILES}") #without this line the list is empty? Schrödinger's list?
    add_custom_target(configure_module_files DEPENDS ${CONFIGURED_MODULE_FILES})
    add_dependencies(vistle_doc configure_module_files)
    message("Configured documentation files: ${CONFIGURED_MODULE_FILES}")

endfunction()

set(VISTLE_DOCUMENTATION_DIR
    "${PROJECT_SOURCE_DIR}"
    CACHE PATH "Path where the documentation will be build")
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
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/clear.py ${VISTLE_DOCUMENTATION_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/conf.py ${VISTLE_DOCUMENTATION_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/mdlink.py ${VISTLE_DOCUMENTATION_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/requirements.txt ${VISTLE_DOCUMENTATION_DIR}/docs
        COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/.readthedocs.yaml ${VISTLE_DOCUMENTATION_DIR})

    copy_readme()

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

macro(add_module_doc_target targetname)

    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
    get_filename_component(CATEGORY ${PARENT_DIR} NAME)
    set(ALL_VISTLE_MODULES_CATEGORY
        ${ALL_VISTLE_MODULES_CATEGORY} ${CATEGORY}
        CACHE INTERNAL "")

    set(VISTLE_DOCUMENTATION_WORKFLOW ${PROJECT_SOURCE_DIR}/doc/tools/generateModuleInfo.vsl)
    #insert module paramerts in markdown files
    message("VISTLE_DOCUMENTATION_SOURCE_DIR ${VISTLE_DOCUMENTATION_SOURCE_DIR}")
    set(DOC_COMMAND
        ${CMAKE_COMMAND} -E env VISTLE_DOCUMENTATION_TARGET=${targetname} VISTLE_BINARY_DIR=${CMAKE_BINARY_DIR}
        VISTLE_MODULE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} VISTLE_DOCUMENTATION_CATEGORY=${CATEGORY} vistle --batch ${VISTLE_DOCUMENTATION_WORKFLOW})

    set(OUTPUT_FILE ${CMAKE_BINARY_DIR}/docs/source/modules/${CATEGORY}/${targetname}.md)
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
        COMMENT "Generating documentation for " ${targetname})
    add_custom_target(${targetname}_doc DEPENDS ${OUTPUT_FILE})

    add_dependencies(vistle_module_doc ${targetname}_doc)

    file(
        GLOB WORKFLOWS
        LIST_DIRECTORIES FALSE
        ${CMAKE_CURRENT_SOURCE_DIR}/*.vsl)
    foreach(file ${WORKFLOWS})
        get_filename_component(workflow ${file} NAME_WLE)
        message("Workflow: ${targetname} ${workflow} ${file}")
        #generate_network_snapshot(${targetname} ${workflow})
        generate_snapshots(${targetname} ${workflow})
    endforeach()
endmacro()

macro(generate_network_snapshot targetname network_file)
    add_custom_command(
        #create a snapshot of the pipeline
        OUTPUT ${CMAKE_CURRENT_LIST_DIR}/${network_file}_workflow.png
        COMMAND vistle --snapshot ${CMAKE_CURRENT_LIST_DIR}/${network_file}_workflow.png ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${VISTLE_DOCUMENTATION_WORKFLOW}.vsl targetname
        COMMENT "Generating network snapshot for " ${network_file}.vsl)

    # add_custom_target(${targetname}_${network_file}_workflow DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${network_file}_workflow.png)
    add_custom_target(${targetname}_${network_file}_workflow DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}_workflow.png)
    add_dependencies(${targetname}_doc ${targetname}_${network_file}_workflow)
endmacro()

macro(generate_snapshots targetname network_file)
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp
    )#if we have a viewpoint file we can generate an result image, only first viewpoint is considered, only first cover is considered
        set(output_dir ${VISTLE_DOCUMENTATION_DIR}/docs/source/pictures)
        add_custom_command(
            OUTPUT ${output_dir}/${network_file}_result.png ${output_dir}/${network_file}_workflow.png
            COMMAND
                ${CMAKE_COMMAND} -E env COCONFIG=${PROJECT_SOURCE_DIR}/doc/config.vistle.doc.xml VISTLE_DOC_IMAGE_NAME=${network_file}
                VISTLE_DOC_SOURCE_DIR=${CMAKE_CURRENT_LIST_DIR} VISTLE_DOC_TARGET_DIR=${VISTLE_DOCUMENTATION_SOURCE_DIR}/pictures vistle
                ${PROJECT_SOURCE_DIR}/doc/tools/resultSnapShot.py
            DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp ${targetname}
                    ${PROJECT_SOURCE_DIR}/doc/tools/resultSnapShot.py
            COMMENT "Generating network and result snapshot for " ${network_file}.vsl)
        add_custom_target(${targetname}_${network_file}_result DEPENDS ${output_dir}/${network_file}_result.png ${output_dir}/${network_file}_workflow.png)
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
