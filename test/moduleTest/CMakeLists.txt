# set minimum cmake version
cmake_minimum_required(VERSION 3.14)

# project name
project(vistle_test LANGUAGES NONE)

# detect python
find_package(Python REQUIRED)

# enable testing functionality
enable_testing()

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/utils/refImageHash.json MY_JSON_STRING)
string(JSON LEN_JSON LENGTH ${MY_JSON_STRING})
math(EXPR LEN_JSON "${LEN_JSON} - 1")
set(tmp_dir ${CMAKE_BINARY_DIR}/test/module_tmp)

# 1. loop over json file entries for relative modulepath
file(REMOVE_RECURSE ${tmp_dir})
if(${LEN_JSON} GREATER_EQUAL 0)
    foreach(IDX RANGE ${LEN_JSON})
        string(JSON MOD MEMBER ${MY_JSON_STRING} ${IDX})
        string(JSON VAL GET ${MY_JSON_STRING} ${MOD})

        get_filename_component(srcDir ${MOD} DIRECTORY [CACHE])
        get_filename_component(network_file ${MOD} NAME_WE [CACHE])
        message("network_file: ${network_file}")
        message("tmp_dir: ${tmp_dir}")

        # 2. Create new image create new imagehash for module
        add_test(${network_file} bash ${CMAKE_CURRENT_SOURCE_DIR}/utils/createAndCompareHash.sh 
                 ${network_file} ${tmp_dir} ${CMAKE_SOURCE_DIR}/${srcDir} ${VAL} ${CMAKE_SOURCE_DIR})
    endforeach()
endif()
