# set minimum cmake version
cmake_minimum_required(VERSION 3.14)

# project name
project(vistle_test LANGUAGES NONE)

# detect python
find_package(Python REQUIRED)

# enable testing functionality

enable_testing()

#TODO:
# 1. loop over json file entries for relative modulepath
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/utils/refImageHash.json MY_JSON_STRING)
string(JSON LEN_JSON LENGTH ${MY_JSON_STRING})
math(EXPR LEN_JSON "${LEN_JSON} - 1")
set(tmp_dir ${CMAKE_BINARY_DIR}/test/module_tmp)
# set(MODULES "")
if(${LEN_JSON} GREATER_EQUAL 0)
    foreach(IDX RANGE ${LEN_JSON})
        string(JSON MOD MEMBER ${MY_JSON_STRING} ${IDX})
        get_filename_component(srcDir ${MOD} DIRECTORY [CACHE])
        # get_filename_component(targetname ${srcDir} NAME [CACHE])
        get_filename_component(network_file ${MOD} NAME_WE [CACHE])
        message("network_file: ${network_file}")
        message("tmp_dir: ${tmp_dir}")
        # 2. Create new image
        set(SNAPSHOT_ARGS "-name ${network_file} -o ${tmp_dir} -sdir ${CMAKE_SOURCE_DIR}/${srcDir}")
        # message("SNAPSHOT_ARGS: ${SNAPSHOT_ARGS}")
        add_test(
            blub
            ${CMAKE_COMMAND}
            -E
            env
            COCONFIG=${CMAKE_SOURCE_DIR}/doc/config.vistle.doc.xml
            SNAPSHOT_ARGS=${SNAPSHOT_ARGS}
            vistle
            ${CMAKE_SOURCE_DIR}/doc/resultSnapShot.py)

        # add_custom_command(COMMAND ${CMAKE_COMMAND} -E env
        #          COCONFIG=${CMAKE_SOURCE_DIR}/doc/config.vistle.doc.xml
        #          SNAPSHOT_ARGS=${SNAPSHOT_ARGS}
        #          vistle ${CMAKE_SOURCE_DIR}/doc/resultSnapShot.py)
        # add_custom_command(
        #     OUTPUT ${tmp_dir}/${network_file}_result.png
        #     COMMAND ${CMAKE_COMMAND} -E env
        #         COCONFIG=${CMAKE_SOURCE_DIR}/doc/config.vistle.doc.xml
        #         SNAPSHOT_ARGS=${SNAPSHOT_ARGS}
        #         vistle ${CMAKE_SOURCE_DIR}/doc/resultSnapShot.py
        #     DEPENDS ${srcDir}/${network_file}.vsl ${srcDir}/${network_file}.vwp
        #             ${CMAKE_SOURCE_DIR}/doc/resultSnapShot.py
        #     COMMENT "Generating result snapshot for " ${network_file}.vsl)
        # add_custom_target(${network_file}_target DEPENDS ${tmp_dir}/${network_file}_result.png)
        # add_dependencies(${CMAKE_CTEST_COMMAND} ${network_file}_target)

        # 3. create new imagehash for module
        set(HASH_ARGS -name ${mod_path}/${network_file}.vsl -sdir ${CMAKE_CURRENT_LIST_DIR}/${network_file}_result.png -jdir
                      ${PROJECT_SOURCE_DIR}/test/moduleTest/utils/refImageHash.json)
        # execute_process(COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/utils/createImageHash.py)
        # 4. compare imagehashes with reference imagehashes
        # list(APPEND MODULES ${MOD})
    endforeach()
endif()
# message("MODULES: ${MODULES}")

# define tests
# add_test(blub ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py)
