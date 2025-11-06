set(VISTLE_DOCUMENTATION_DIR
    "${PROJECT_BINARY_DIR}"
    CACHE PATH "Path where the documentation will be built (i.e. working copy of github.com/vistle/vistle.github.io)")

set(VISTLE_DOCUMENTATION_SOURCE_DIR ${VISTLE_DOCUMENTATION_DIR}/docs)
set(GENDOC "${PROJECT_BINARY_DIR}/gendoc")
set(TOOLDIR "${PROJECT_SOURCE_DIR}/doc/build/tools")

set(MODULE_DOC_FILES
    ""
    CACHE INTERNAL "")

macro(configure_documentation_detail INPUT_FILE OUTPUT_FILE TARGET)
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

    list(APPEND ${TARGET} ${OUTPUT_FILE})
endmacro()

# transform string to titlecase
# e.g. "this is a test" -> "This Is a Test"
function(titlecase INPUT OUTPUT_VAR)
    execute_process(
        COMMAND "${Python_EXECUTABLE}" "${TOOLDIR}/titlecase.py" "${INPUT}"
        OUTPUT_VARIABLE TITLE
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${OUTPUT_VAR}
        ${TITLE}
        PARENT_SCOPE)
endfunction()

# create an alphabetic index referencing the modules in a category
macro(configure_category_index name TARGET)
    string(TOLOWER ${name} lower)
    set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/${lower}/index.rst)
    set(CATEGORYNAME ${lower})
    set(CATEGORYTITLE ${name})

    titlecase("${VISTLE_CATEGORY_${name}_DESCRIPTION}" CATEGORYDESC)
    set(CATEGORYMODULES)
    set(CATEGORYMODULESTOC)
    set(FIRSTLETTER)

    foreach(mod IN LISTS VISTLE_CATEGORY_${name}_MODULES)
        list(FIND ALL_MODULES ${mod} idx)
        if(idx EQUAL -1)
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
endmacro()

# create an alphabetic index referencing all modules
macro(configure_all_modules TARGET)
    set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/all.rst)
    set(CATEGORYNAME All)
    set(CATEGORYTITLE "All Modules")
    set(CATEGORYDESC "all modules (and their category), sorted alphabetically")
    set(CATEGORYMODULES)
    set(CATEGORYMODULESTOC)
    set(FIRSTLETTER)
    set(ALL ${ALL_MODULES})
    list(SORT ALL)

    foreach(mod IN LISTS ALL)
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
endmacro()

# create an index file referencing all categories with their modules
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
        string(APPEND CATEGORIESLONG "* :doc:`${cat} - ${desc} <${lower}/index>`\n")
        string(APPEND CATEGORIESSHORT "   ${cat} <${lower}/index.rst>\n")
    endforeach()

    configure_file(${PROJECT_SOURCE_DIR}/doc/module/index.rst.in ${OUTPUT_FILE} @ONLY)
    list(APPEND ${TARGET} ${OUTPUT_FILE})
endmacro()

# create a single document describing all categories
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
endmacro()

function(configure_documentation)
    # transform [Module]() to [](project:#mod-Module)
    set(INSERT_LINKS
        ${CMAKE_COMMAND}
        -E
        env
        ${Python_EXECUTABLE}
        ${TOOLDIR}/insertModuleLinks.py
        ${MODULE_DESCRIPTION_FILE}
        ${PROJECT_SOURCE_DIR}
        ${VISTLE_DOCUMENTATION_SOURCE_DIR})

    # create all the indices
    set(CONFIGURED_FILES)
    foreach(cat IN LISTS ALL_CATEGORIES_ORDERED)
        configure_category_index(${cat} CONFIGURED_FILES)

        set(INPUT_FILE ${VISTLE_CATEGORY_${cat}_FILE})
        cmake_path(GET INPUT_FILE FILENAME FILE)
        string(TOLOWER ${cat} lower)
        set(OUTPUT_FILE ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/${lower}/${FILE})
        configure_documentation_detail(${INPUT_FILE} ${OUTPUT_FILE} CONFIGURED_FILES)
    endforeach()
    configure_all_modules(CONFIGURED_FILES)
    configure_modules_index(CONFIGURED_FILES)
    configure_categories_overview(CONFIGURED_FILES)
    add_custom_target(configure_documentation_files DEPENDS ${CONFIGURED_FILES})
    add_dependencies(vistle_doc configure_documentation_files)

    # update module descriptions with links to other modules
    set(CONFIGURED_MODULE_FILES)
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

set(RUN_VISTLE COVER_TERMINATE_SESSION=1 vistle -q --vrb=no)
macro(add_module_doc_target modulename CATEGORY)
    string(TOLOWER ${CATEGORY} category)
    set(MODULE ${modulename})
    set(RELNAME module/${category}/${modulename}/${modulename}.md)
    list(APPEND MODULE_DOC_FILES ${RELNAME})
    set(MODULE_DOC_FILES
        ${MODULE_DOC_FILES}
        CACHE INTERNAL "")

    set(VISTLE_DOCUMENTATION_WORKFLOW ${TOOLDIR}/generateModuleInfo.vsl)

    # insert module parameters in markdown files
    set(OUTPUT_FILE ${GENDOC}/${RELNAME})
    set(INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${modulename}.md)

    if(NOT EXISTS ${INPUT_FILE})
        set(INPUT_FILE)
    endif()

    set(DOC_COMMAND
        ${CMAKE_COMMAND} -E env VISTLE_DOCUMENTATION_TARGET=${modulename} VISTLE_BINARY_DIR=${CMAKE_BINARY_DIR}
        VISTLE_MODULE_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} VISTLE_DOCUMENTATION_CATEGORY=${category}
        VISTLE_DOCUMENTATION_SOURCE_DIR=${VISTLE_DOCUMENTATION_SOURCE_DIR}
        VISTLE_LOGFILE=${PROJECT_BINARY_DIR}/logs/vistle_doc/geninfo-${modulename}_{port}_{id}_{name}.log ${RUN_VISTLE} --batch
        ${VISTLE_DOCUMENTATION_WORKFLOW})

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${DOC_COMMAND}
        DEPENDS # build if changes in:
                ${INPUT_FILE} # the custom documentation
                ${modulename} # the module's source code
                ${VISTLE_DOCUMENTATION_WORKFLOW} # the file that gets loaded by vistle to generate the documentation
        COMMENT "Documentation - generating for ${modulename}")
    add_custom_target(${modulename}_module_doc DEPENDS ${OUTPUT_FILE})
    add_dependencies(vistle_module_doc ${modulename}_module_doc)

    file(
        GLOB WORKFLOWS
        LIST_DIRECTORIES FALSE
        ${CMAKE_CURRENT_SOURCE_DIR}/*.vsl)

    foreach(file ${WORKFLOWS})
        get_filename_component(workflow ${file} NAME_WLE)
        #message("Workflow: ${modulename} ${workflow} ${file}")

        set(output_dir ${VISTLE_DOCUMENTATION_SOURCE_DIR}/module/${category}/${modulename})

        # generating the result and workflow snapshots from different vistle instances
        # prevents vistle from asking to save the workflow
        generate_result_snapshot(${modulename} ${workflow} ${output_dir})
        generate_network_snapshot_and_quit(${modulename} ${workflow} ${output_dir})
        set(out ${output_dir}/${workflow}.vsl)
        set(in ${CMAKE_CURRENT_LIST_DIR}/${workflow}.vsl)
        add_custom_command(
            OUTPUT ${out}
            COMMAND ${CMAKE_COMMAND} -E copy ${in} ${out}
            DEPENDS ${in} ${modulename}
            COMMENT "Documentation - copying workflow: ${in} -> ${out}")
        set(custom_target ${modulename}_${workflow}_copy)
        add_custom_target(${custom_target} DEPENDS ${out})
        add_dependencies(vistle_module_doc ${custom_target})
    endforeach()
endmacro()

macro(generate_snapshot_base modulename network_file output_dir workflow result)
    set(viewpoint ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vwp)

    set(batch "")

    if(NOT ${workflow})
        set(batch "--batch")
    endif()

    set(VISTLE_DOC_ARGS)
    set(output_files)
    set(custom_target ${modulename}_${network_file})

    if(${result} AND NOT EXISTS ${viewpoint})
        message(WARNING "Cannot generate snapshot for ${modulename} - ${network_file}: missing viewpoint file \"${viewpoint}\"")
    elseif(${result})
        # if we have a viewpoint file we can generate a result image, only first viewpoint is considered, only first cover is considered
        set(VISTLE_DOC_ARGS ${VISTLE_DOC_ARGS} result)
        list(APPEND output_files ${output_dir}/${network_file}_result.png)
        string(APPEND custom_target _result)
    endif()

    if(${workflow})
        set(VISTLE_DOC_ARGS ${VISTLE_DOC_ARGS} workflow)
        list(APPEND output_files ${output_dir}/${network_file}_workflow.png)
        string(APPEND custom_target _workflow)
    endif()

    if(${result} OR ${workflow})
        set(coconfig ${PROJECT_SOURCE_DIR}/doc/build/config.vistle.doc.xml)
        set(script ${TOOLDIR}/snapShot.py)
        add_custom_command(
            OUTPUT ${output_files}
            COMMAND
                ${CMAKE_COMMAND} -E env VISTLE_DOC_IMAGE_NAME=${network_file} VISTLE_DOC_SOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}
                VISTLE_DOC_TARGET_DIR=${output_dir} VISTLE_DOC_ARGS=${VISTLE_DOC_ARGS} COCONFIG=${coconfig}
                VISTLE_LOGFILE=${PROJECT_BINARY_DIR}/logs/vistle_doc/snapshot-${network_file}_{port}_{id}_{name}.log ${RUN_VISTLE} ${batch} ${script}
            DEPENDS ${modulename}
                    ${CMAKE_CURRENT_LIST_DIR}/${network_file}.vsl
                    ${viewpoint}
                    ${script}
                    ${coconfig}
                    ${ALL_MODULES}
                    vistle_manager
                    vistle_gui
            COMMENT "Generating network and result snapshot for ${network_file}.vsl")
        add_custom_target(${custom_target} DEPENDS ${output_files})
        add_dependencies(${modulename}_module_doc ${custom_target})
    endif()
endmacro()

macro(generate_result_snapshot modulename network_file output_dir)
    generate_snapshot_base(${modulename} ${network_file} ${output_dir} FALSE TRUE)
endmacro()

macro(generate_network_snapshot_and_quit modulename network_file output_dir)
    generate_snapshot_base(${modulename} ${network_file} ${output_dir} TRUE FALSE)
endmacro()

add_custom_target(vistle_module_doc)
add_custom_target(vistle_doc)
add_dependencies(vistle_doc vistle_module_doc)

add_custom_target(docs) # add a short alias
add_dependencies(docs vistle_doc)
