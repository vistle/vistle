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
        string(JSON VAL GET ${MY_JSON_STRING} ${MOD})

        get_filename_component(srcDir ${MOD} DIRECTORY [CACHE])
        # get_filename_component(targetname ${srcDir} NAME [CACHE])
        get_filename_component(network_file ${MOD} NAME_WE [CACHE])
        message("network_file: ${network_file}")
        message("tmp_dir: ${tmp_dir}")
        # 2. Create new image
        set(SNAPSHOT_ARGS "-name ${network_file} -o ${tmp_dir} -sdir ${CMAKE_SOURCE_DIR}/${srcDir}")
        # message("SNAPSHOT_ARGS: ${SNAPSHOT_ARGS}")
        add_test(
            "${network_file}_snapshot"
            ${CMAKE_COMMAND}
            -E
            env
            COCONFIG=${CMAKE_SOURCE_DIR}/doc/config.vistle.doc.xml
            SNAPSHOT_ARGS=${SNAPSHOT_ARGS}
            vistle
            ${CMAKE_SOURCE_DIR}/doc/resultSnapShot.py)


        # 3. create new imagehash for module
        set(HASH_ARGS "-name ${network_file} -sdir ${tmp_dir}/${network_file}_result.png -cmp -refHashString ${VAL}")
        add_test(
            "${network_file}_compareHash"
            ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/utils/createImageHash.py -name ${network_file} -sdir ${tmp_dir}/${network_file}_result.png -cmp -refHashString ${VAL})

    endforeach()
endif()
