set(VISTLE_DOCUMENTATION_DIR
    "${PROJECT_BINARY_DIR}"
    CACHE PATH "Path where the documentation will be built")

set(GENDOC "${PROJECT_BINARY_DIR}/gendoc")

set(ALL_VISTLE_MODULES_CATEGORY
    ""
    CACHE INTERNAL "")
set(ADDITIONAL_DOCUMENTATION_SOURCES
    ""
    CACHE INTERNAL "Markdown files that belong to the source tree and are used in the documentation")

set(MODULE_DOC_FILES
    ""
    CACHE INTERNAL "")

macro(add_documentation_source)
    foreach(source ${ARGN})
        set(ADDITIONAL_DOCUMENTATION_SOURCES
            ${ADDITIONAL_DOCUMENTATION_SOURCES} ${CMAKE_CURRENT_LIST_DIR}/${source}
            CACHE INTERNAL "Markdown files that belong to the source tree and are used in the documentation")
    endforeach()
endmacro()

# these modules should not get documentation
set(IGNR_MODS "COVER_plugin")

set(TOOLDIR "${PROJECT_SOURCE_DIR}/doc/build/tools")

function(titlecase INPUT OUTPUT_VAR)
    execute_process(
        COMMAND "${Python_EXECUTABLE}" "${TOOLDIR}/titlecase.py" "${INPUT}"
        OUTPUT_VARIABLE TITLE
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${OUTPUT_VAR}
        ${TITLE}
        PARENT_SCOPE)
endfunction()

macro(configure_documentation_detail INPUT_FILE OUTPUT_FILE TARGET)
    list(APPEND ${TARGET} ${OUTPUT_FILE})
    if(${INPUT_FILE} MATCHES ".*\\.md$")
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${INSERT_LINKS} ${INPUT_FILE} ${OUTPUT_FILE}
            DEPENDS ${INPUT_FILE} vistle_module_doc ${TOOLDIR}/insertModuleLinks.py
            COMMENT "Documentation - configuring: ${OUTPUT_FILE}")
    else()
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy ${INPUT_FILE} ${OUTPUT_FILE}
            DEPENDS ${INPUT_FILE}
            COMMENT "Documentation - copying: ${OUTPUT_FILE}")
    endif()
endmacro()

macro(configure_category_index name TARGET)
    string(TOLOWER ${name} lower)
    set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/${lower}/index.rst)
    set(CATEGORYNAME ${lower})
    set(CATEGORYTITLE ${name})
    #set(CATEGORYDESC ${VISTLE_CATEGORY_${name}_DESCRIPTION})
    titlecase("${VISTLE_CATEGORY_${name}_DESCRIPTION}" CATEGORYDESC)
    set(CATEGORYMODULES)
    set(CATEGORYMODULESTOC)
    set(FIRSTLETTER)
    foreach(mod IN LISTS VISTLE_CATEGORY_${name}_MODULES)
        list(FIND IGNR_MODS ${mod} idx)
        if(NOT idx EQUAL -1)
            continue()
        endif()
        set(desc ${VISTLE_MODULE_${mod}_DESCRIPTION})
        string(APPEND CATEGORYMODULESTOC "   ${mod} <${mod}/${mod}.md>\n")

        string(SUBSTRING ${mod} 0 1 F)
        if(NOT "${F}" STREQUAL "${FIRSTLETTER}")
            set(FIRSTLETTER ${F})
            string(APPEND CATEGORYMODULES "\n**${FIRSTLETTER}**\n")
        endif()
        string(APPEND CATEGORYMODULES "   :doc:`${mod} <${mod}/${mod}>` *${desc}*\n\n")
    endforeach()
    configure_file(${PROJECT_SOURCE_DIR}/doc/module/category_index.rst.in ${OUTPUT_FILE} @ONLY)
    list(APPEND ${TARGET} ${OUTPUT_FILE})
    #add_custom_command( OUTPUT ${OUTPUT_FILE} COMMAND ${INSERT_LINKS} ${INPUT_FILE} ${OUTPUT_FILE} DEPENDS ${INPUT_FILE} vistle_module_doc COMMENT "Configuring file: ${OUTPUT_FILE}")
endmacro()

macro(configure_all_modules TARGET)
    set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/all.rst)
    set(CATEGORYNAME ${all})
    set(CATEGORYTITLE "All Modules")
    set(CATEGORYDESC "All Modules")
    set(CATEGORYDESC)
    set(CATEGORYMODULES)
    set(CATEGORYMODULESTOC)
    set(FIRSTLETTER)
    set(ALL ${ALL_MODULES})
    list(SORT ALL)
    foreach(mod IN LISTS ALL)
        list(FIND IGNR_MODS ${mod} idx)
        if(NOT idx EQUAL -1)
            continue()
        endif()

        set(desc ${VISTLE_MODULE_${mod}_DESCRIPTION})
        set(CAT ${VISTLE_MODULE_${mod}_CATEGORY})
        string(TOLOWER ${CAT} cat)

        string(SUBSTRING ${mod} 0 1 F)
        if(NOT "${F}" STREQUAL "${FIRSTLETTER}")
            set(FIRSTLETTER ${F})
            string(APPEND CATEGORYMODULES "\n**${FIRSTLETTER}**\n")
        endif()
        string(APPEND CATEGORYMODULES "   :doc:`${mod} <${cat}/${mod}/${mod}>` (:doc:`${CAT} <${cat}/index>`) *${desc}*\n\n")
    endforeach()
    configure_file(${PROJECT_SOURCE_DIR}/doc/module/category_index.rst.in ${OUTPUT_FILE} @ONLY)
    list(APPEND ${TARGET} ${OUTPUT_FILE})
    #add_custom_command( OUTPUT ${OUTPUT_FILE} COMMAND ${INSERT_LINKS} ${INPUT_FILE} ${OUTPUT_FILE} DEPENDS ${INPUT_FILE} vistle_module_doc COMMENT "Configuring file: ${OUTPUT_FILE}")
endmacro()

macro(configure_modules_index TARGET)
    set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/index.rst)
    set(CATEGORIESSHORT)
    set(CATEGORIESLONG)
    foreach(cat IN LISTS ALL_CATEGORIES_ORDERED)
        if(NOT VISTLE_CATEGORY_${cat}_MODULES)
            continue()
        endif()
        string(TOLOWER ${cat} lower)
        set(desc ${VISTLE_CATEGORY_${cat}_DESCRIPTION})
        #string(APPEND CATEGORIESLONG "* :doc:`${cat} - ${desc} <${lower}/index>`\n")
        string(APPEND CATEGORIESLONG "* :doc:`${cat} - ${desc} <${lower}/index>`\n")
        string(APPEND CATEGORIESSHORT "   ${cat} <${lower}/index.rst>\n")
    endforeach()
    configure_file(${PROJECT_SOURCE_DIR}/doc/module/index.rst.in ${OUTPUT_FILE} @ONLY)
    list(APPEND ${TARGET} ${OUTPUT_FILE})
    #add_custom_command( OUTPUT ${OUTPUT_FILE} COMMAND ${INSERT_LINKS} ${INPUT_FILE} ${OUTPUT_FILE} DEPENDS ${INPUT_FILE} vistle_module_doc COMMENT "Configuring file: ${OUTPUT_FILE}")
endmacro()

macro(configure_categories_overview TARGET)
    set(TEMP_FILE ${GENDOC}/module/categories.md)
    set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/categories.md)
    set(CATEGORIES)
    foreach(cat IN LISTS ALL_CATEGORIES_ORDERED)
        if(NOT VISTLE_CATEGORY_${cat}_MODULES)
            continue()
        endif()
        string(TOLOWER ${cat} lower)
        set(desc ${VISTLE_CATEGORY_${cat}_DESCRIPTION})
        set(file ${VISTLE_CATEGORY_${cat}_FILE})
        string(APPEND CATEGORIES "\n")
        string(APPEND CATEGORIES "## ${cat}\n")
        file(READ ${file} TEXT)
        string(APPEND CATEGORIES "\n${TEXT}\n\n")
    endforeach()
    configure_file(${PROJECT_SOURCE_DIR}/doc/module/categories.md.in ${TEMP_FILE} @ONLY)
    configure_documentation_detail(${TEMP_FILE} ${OUTPUT_FILE} ${TARGET})
    #list(APPEND ${TARGET} ${OUTPUT_FILE})
    #add_custom_command( OUTPUT ${OUTPUT_FILE} COMMAND ${INSERT_LINKS} ${INPUT_FILE} ${OUTPUT_FILE} DEPENDS ${INPUT_FILE} vistle_module_doc COMMENT "Configuring file: ${OUTPUT_FILE}")
endmacro()

function(configure_documentation)
    set(INSERT_LINKS
        ${CMAKE_COMMAND}
        -E
        env
        ALL_VISTLE_MODULES="${ALL_MODULES}"
        ALL_VISTLE_MODULES_CATEGORY="${ALL_VISTLE_MODULES_CATEGORY}"
        ${Python_EXECUTABLE}
        ${TOOLDIR}/insertModuleLinks.py
        ${PROJECT_SOURCE_DIR}
        ${VISTLE_DOCUMENTATION_SOURCE_DIR})

    # List to hold all the output files
    set(CONFIGURED_MODULE_FILES)
    set(CONFIGURED_FILES)
    # Find all files in the ToInstall directory recursively
    set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/doc)
    set(DOCUMENTATION_FILES index.rst)
    foreach(
        DIR
        module
        intro
        quickstart
        develop
        architecture
        publications
        gallery)
        file(
            GLOB_RECURSE DIRFILES
            RELATIVE ${SOURCE_DIR}
            "${SOURCE_DIR}/${DIR}/*.md" "${SOURCE_DIR}/${DIR}/*.rst" "${SOURCE_DIR}/${DIR}/*.png" "${SOURCE_DIR}/${DIR}/*.jpeg" "${SOURCE_DIR}/${DIR}/*.jpg")
        list(APPEND DOCUMENTATION_FILES ${DIRFILES})
    endforeach()

    foreach(DOCUMENTATION_FILE ${DOCUMENTATION_FILES})
        set(INPUT_FILE ${SOURCE_DIR}/${DOCUMENTATION_FILE})
        set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/${DOCUMENTATION_FILE})
        configure_documentation_detail(${INPUT_FILE} ${OUTPUT_FILE} CONFIGURED_FILES)
        message("configure_documentation_detail(${INPUT_FILE} ${OUTPUT_FILE} CONFIGURED_FILES)")
    endforeach()

    # Configure each file and add to the list of output files

    foreach(cat IN LISTS ALL_CATEGORIES_ORDERED)
        configure_category_index(${cat} CONFIGURED_FILES)

        set(INPUT_FILE ${VISTLE_CATEGORY_${cat}_FILE})
        list(APPEND DOCUMENTATION_FILES ${INPUT_FILE})
        cmake_path(GET INPUT_FILE FILENAME FILE)
        string(TOLOWER ${cat} lower)
        set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/${lower}/${FILE})
        configure_documentation_detail(${INPUT_FILE} ${OUTPUT_FILE} CONFIGURED_FILES)
    endforeach()
    configure_all_modules(CONFIGURED_FILES)
    configure_modules_index(CONFIGURED_FILES)
    configure_categories_overview(CONFIGURED_FILES)

    #add_custom_target(documentation_files DEPENDS ${DOCUMENTATION_FILES})
    #add_dependencies(vistle_doc documentation_files)

    add_custom_target(configure_documentation_files DEPENDS ${CONFIGURED_FILES})
    add_dependencies(vistle_doc configure_documentation_files)

    # Custom command to copy specified documentation files from source tree
    set(CONFIGURED_FILES_FROM_SOURCE_TREE "")
    foreach(README_FILE ${ADDITIONAL_DOCUMENTATION_SOURCES})
        file(RELATIVE_PATH RELATIVE_PATH ${CMAKE_CURRENT_LIST_DIR} ${README_FILE})
        message("ADD doc: ${README_FILE} ${RELATIVE_PATH}")
        set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/readme/${RELATIVE_PATH})
        configure_documentation_detail(${README_FILE} ${OUTPUT_FILE} CONFIGURED_FILES_FROM_SOURCE_TREE)
    endforeach()
    add_custom_target(copy_readme_files DEPENDS ${CONFIGURED_FILES_FROM_SOURCE_TREE})
    add_dependencies(vistle_doc copy_readme_files)

    list(APPEND MODULE_DOC_FILES)
    foreach(MOD IN LISTS MODULE_DOC_FILES)
        set(INPUT_FILE ${GENDOC}/${MOD})
        set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/${MOD})
        list(APPEND CONFIGURED_MODULE_FILES ${OUTPUT_FILE})
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${INSERT_LINKS} ${INPUT_FILE} ${OUTPUT_FILE}
            DEPENDS ${INPUT_FILE} vistle_module_doc ${TOOLDIR}/insertModuleLinks.py
            COMMENT "Documentation - configuring: ${OUTPUT_FILE}")
    endforeach()
    add_custom_target(configure_module_documentation_files DEPENDS ${CONFIGURED_MODULE_FILES})
    add_dependencies(vistle_doc configure_module_documentation_files)
endfunction()

macro(append_module_category targetname category)

    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
    get_filename_component(CATEGORY ${PARENT_DIR} NAME)
    set(ALL_VISTLE_MODULES_CATEGORY
        ${ALL_VISTLE_MODULES_CATEGORY} ${category}
        CACHE INTERNAL "")

endmacro()

macro(add_module_doc_target targetname CATEGORY)
    string(TOLOWER ${CATEGORY} category)
    set(MODULE ${targetname})
    set(RELNAME module/${category}/${targetname}/${targetname}.md)
    list(APPEND MODULE_DOC_FILES ${RELNAME})
    set(MODULE_DOC_FILES
        ${MODULE_DOC_FILES}
        CACHE INTERNAL "")

    set(VISTLE_DOCUMENTATION_WORKFLOW ${TOOLDIR}/generateModuleInfo.vsl)
    #insert module paramerts in markdown files
    set(OUTPUT_FILE ${GENDOC}/${RELNAME})
    set(INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${targetname}.md)
    if(NOT EXISTS ${INPUT_FILE})
        set(INPUT_FILE)
    endif()

    set(DOC_COMMAND
        ${CMAKE_COMMAND} -E env VISTLE_DOCUMENTATION_TARGET=${targetname} VISTLE_BINARY_DIR=${CMAKE_BINARY_DIR}
        VISTLE_MODULE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} VISTLE_DOCUMENTATION_CATEGORY=${category}
        VISTLE_DOCUMENTATION_SOURCE_DIR=${VISTLE_DOCUMENTATION_SOURCE_DIR} vistle -q --vrb=no --batch ${VISTLE_DOCUMENTATION_WORKFLOW})

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${DOC_COMMAND}
        DEPENDS #build if changes in:
                ${INPUT_FILE} #the custom documentation
                ${targetname} #the module's source code
                ${VISTLE_DOCUMENTATION_WORKFLOW} #the file that gets loaded by vistle to generate the documentation
                ${DOCUMENTATION_DEPENDENCIES} #custom dependencies set by the calling module
        COMMENT "Documentation - generating for ${targetname}")
    add_custom_target(${targetname}_doc DEPENDS ${OUTPUT_FILE})

    add_dependencies(vistle_module_doc ${targetname}_doc)

    file(
        GLOB WORKFLOWS
        LIST_DIRECTORIES FALSE
        ${CMAKE_CURRENT_SOURCE_DIR}/*.vsl)
    foreach(file ${WORKFLOWS})
        get_filename_component(workflow ${file} NAME_WLE)
        message("Workflow: ${targetname} ${workflow} ${file}")

        set(output_dir ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/${category}/${targetname})
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
        add_custom_command(
            OUTPUT ${output_file}
            COMMAND
                ${CMAKE_COMMAND} -E env COCONFIG=${PROJECT_SOURCE_DIR}/doc/build/config.vistle.doc.xml VISTLE_DOC_IMAGE_NAME=${network_file}
                VISTLE_DOC_SOURCE_DIR=${CMAKE_CURRENT_LIST_DIR} VISTLE_DOC_TARGET_DIR=${output_dir} VISTLE_DOC_ARGS=${VISTLE_DOC_ARGS} vistle -q --vrb=no
                ${batch} ${TOOLDIR}/snapShot.py
            DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp ${targetname} ${TOOLDIR}/snapShot.py
            COMMENT "Generating network and result snapshot for ${network_file}.vsl")
        add_custom_target(${custom_target} DEPENDS ${output_file})
        add_dependencies(${targetname}_doc ${custom_target})
    else()
        message(
            WARNING
                "Cannot generate snapshots for ${targetname} - ${network_file}: missing viewpoint file, make sure a viewpoint file named \"${network_file}.vwp\" exists"
        )
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
        COMMAND vistle -q --vrb=no --snapshot ${output_dir}/${network_file}_workflow.png ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl ${targetname}
        COMMENT "Generating network snapshot for ${network_file}.vsl")

    add_custom_target(${targetname}_${network_file}_workflow DEPENDS ${output_dir}/${network_file}_workflow.png)
    add_dependencies(${targetname}_doc ${targetname}_${network_file}_workflow)
endmacro()

add_custom_target(vistle_module_doc)
add_custom_target(vistle_doc)
add_dependencies(vistle_doc vistle_module_doc)

add_custom_target(docs) # add a short alias
add_dependencies(docs vistle_doc)

set(READTHEDOCS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/doc/build/readthedocs)
set(VISTLE_DOCUMENTATION_SOURCE_DIR ${VISTLE_DOCUMENTATION_DIR}/docs)

#copy the readthedocs configuration scripts
add_custom_command(
    TARGET vistle_doc
    COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/conf.py ${VISTLE_DOCUMENTATION_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/mdlink.py ${VISTLE_DOCUMENTATION_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/html_image_processor.py ${VISTLE_DOCUMENTATION_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/requirements.txt ${VISTLE_DOCUMENTATION_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${READTHEDOCS_SOURCE_DIR}/.readthedocs.yaml ${VISTLE_DOCUMENTATION_SOURCE_DIR})

vistle_find_package(Sphinx)
if(SPHINX_EXECUTABLE)
    add_custom_command(TARGET vistle_doc COMMAND ${CMAKE_COMMAND} -E make_directory ${VISTLE_DOCUMENTATION_SOURCE_DIR}/build)
    add_custom_command(
        TARGET vistle_doc
        COMMAND ${SPHINX_EXECUTABLE} -M html . build
        WORKING_DIRECTORY ${VISTLE_DOCUMENTATION_SOURCE_DIR}
        COMMENT "Building Read the Docs documentation" DEPENDS vistle_module_doc)
else(SPHINX_EXECUTABLE)
    message("Sphinx (sphinx-build) not found, documentation cannot be built")
endif(SPHINX_EXECUTABLE)
