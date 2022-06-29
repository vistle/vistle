## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

set(ISPC_INCLUDE_DIR "")
macro(INCLUDE_DIRECTORIES_ISPC)
    set(ISPC_INCLUDE_DIR ${ISPC_INCLUDE_DIR} ${ARGN})
endmacro()

if(ENABLE_ISPC_SUPPORT)

    set(ISPC_VERSION_REQUIRED "1.7.1")

    if(NOT ISPC_EXECUTABLE)
        find_program(ISPC_EXECUTABLE ispc DOC "Path to the ISPC executable.")
        if(NOT ISPC_EXECUTABLE)
            message(FATAL_ERROR "Intel SPMD Compiler (ISPC) not found. Disable ENABLE_ISPC_SUPPORT or install ISPC.")
        else()
            message(STATUS "Found Intel SPMD Compiler (ISPC): ${ISPC_EXECUTABLE}")
        endif()
    endif()

    if(NOT ISPC_VERSION)
        execute_process(COMMAND ${ISPC_EXECUTABLE} --version OUTPUT_VARIABLE ISPC_OUTPUT)
        string(REGEX MATCH " ([0-9]+[.][0-9]+[.][0-9]+)(dev)? " DUMMY "${ISPC_OUTPUT}")
        set(ISPC_VERSION ${CMAKE_MATCH_1})

        if(ISPC_VERSION VERSION_LESS ISPC_VERSION_REQUIRED)
            message(FATAL_ERROR "Need at least version ${ISPC_VERSION_REQUIRED} of Intel SPMD Compiler (ISPC).")
        endif()

        set(ISPC_VERSION
            ${ISPC_VERSION}
            CACHE STRING "ISPC Version")
        mark_as_advanced(ISPC_VERSION)
        mark_as_advanced(ISPC_EXECUTABLE)
    endif()

    get_filename_component(ISPC_DIR ${ISPC_EXECUTABLE} PATH)

    if(VISTLE_64BIT_INDICES)
        set(VISTLE_ISPC_ADDRESSING
            64
            CACHE STRING "32 vs 64 bit addressing in ispc")
    else()
        set(VISTLE_ISPC_ADDRESSING
            32
            CACHE STRING "32 vs 64 bit addressing in ispc")
    endif()
    mark_as_advanced(VISTLE_ISPC_ADDRESSING)

    macro(ispc_compile ISPC_TARGET)
        set(ISPC_ADDITIONAL_ARGS "${ISPC_COMPILE_FLAGS}")
        if(VISTLE_TIME_BUILD)
            set(ISPC_ADDITIONAL_ARGS "${ISPC_ADDITIONAL_ARGS}" --time-trace)
        endif()
        if(VISTLE_COLOR_DIAGNOSTICS)
            set(ISPC_ADDITIONAL_ARGS "${ISPC_ADDITIONAL_ARGS}" --colored-output)
        endif()

        if(__XEON__)
            set(ISPC_TARGET_EXT ${CMAKE_CXX_OUTPUT_EXTENSION})
        else()
            set(ISPC_TARGET_EXT .cpp)
            set(ISPC_ADDITIONAL_ARGS ${ISPC_ADDITIONAL_ARGS} --opt=force-aligned-memory)
        endif()

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(OPTS -O0 -g)
        elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
            set(OPTS -O2 -DNDEBUG)
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            set(OPTS -O3 --math-lib=fast --opt=fast-math -DNDEBUG)
        else()
            set(OPTS -O3 --math-lib=fast --opt=fast-math -g)
        endif()

        if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
            set(ISPC_ARCHITECTURE "x86-64")
        endif()
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
            set(ISPC_ARCHITECTURE "aarch64")
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(ISPC_ARCHITECTURE "x86-64")
        else()
            set(ISPC_ARCHITECTURE "x86")
        endif()
        #this does not work on windows
        message(${ISPC_ARCHITECTURE})

        set(ISPC_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR})
        include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${ISPC_TARGET_DIR})

        if(ISPC_INCLUDE_DIR)
            string(REPLACE ";" ";-I;" ISPC_INCLUDE_DIR_PARMS "${ISPC_INCLUDE_DIR}")
            set(ISPC_INCLUDE_DIR_PARMS "-I" ${ISPC_INCLUDE_DIR_PARMS})
        endif()

        if(__XEON__)
            string(REPLACE ";" "," ISPC_TARGET_ARGS "${ISPC_TARGETS}")
        else()
            set(ISPC_TARGET_ARGS generic-16)
            set(ISPC_ADDITIONAL_ARGS ${ISPC_ADDITIONAL_ARGS} --emit-c++ -D__XEON_PHI__ --c++-include-file=${ISPC_DIR}/examples/intrinsics/knc.h)
        endif()

        set(ISPC_OBJECTS "")

        foreach(src ${ARGN})
            get_filename_component(fname ${src} NAME_WE)
            get_filename_component(dir ${src} PATH)

            if("${dir}" STREQUAL "")
                set(outdir ${ISPC_TARGET_DIR})
            else("${dir}" STREQUAL "")
                set(outdir ${ISPC_TARGET_DIR}/${dir})
            endif("${dir}" STREQUAL "")
            set(outdirh ${ISPC_TARGET_DIR})

            set(deps "")
            if(EXISTS ${outdir}/${fname}.dev.idep)
                file(READ ${outdir}/${fname}.dev.idep contents)
                string(REPLACE " " ";" contents "${contents}")
                string(REPLACE ";" "\\\\;" contents "${contents}")
                string(REPLACE "\n" ";" contents "${contents}")
                foreach(dep ${contents})
                    if(EXISTS ${dep})
                        set(deps ${deps} ${dep})
                    endif(EXISTS ${dep})
                endforeach(dep ${contents})
            endif()

            set(results "${outdir}/${fname}.dev${ISPC_TARGET_EXT}")

            # if we have multiple targets add additional object files
            if(__XEON__)
                list(LENGTH ISPC_TARGETS NUM_TARGETS)
                if(NUM_TARGETS GREATER 1)
                    foreach(target ${ISPC_TARGETS})
                        # in v1.9.0 ISPC changed the ISA suffix of avx512knl-i32x16 to just 'avx512knl'
                        if(${target} STREQUAL "avx512knl-i32x16" AND NOT ISPC_VERSION VERSION_LESS "1.9.0")
                            set(target "avx512knl")
                        elseif(${target} STREQUAL "avx512skx-i32x16")
                            set(target "avx512skx")
                        endif()
                        set(results ${results} "${outdir}/${fname}.dev_${target}${ISPC_TARGET_EXT}")
                    endforeach()
                endif()
            endif()

            if(NOT WIN32)
                set(ISPC_ADDITIONAL_ARGS ${ISPC_ADDITIONAL_ARGS} --pic)
            endif()

            add_custom_command(
                OUTPUT ${results} ${outdirh}/${fname}_ispc.h
                COMMAND ${CMAKE_COMMAND} -E make_directory ${outdir}
                COMMAND
                    ${ISPC_EXECUTABLE} ${ISPC_INCLUDE_DIR_PARMS} -I ${CMAKE_CURRENT_SOURCE_DIR} --arch=${ISPC_ARCHITECTURE}
                    --addressing=${VISTLE_ISPC_ADDRESSING} --target=${ISPC_TARGET_ARGS} ${OPTS}
                    #--wno-perf
                    ${ISPC_ADDITIONAL_ARGS} -h ${outdirh}/${fname}_ispc.h -MMM ${outdir}/${fname}.dev.idep -o ${outdir}/${fname}.dev${ISPC_TARGET_EXT}
                    ${CMAKE_CURRENT_SOURCE_DIR}/${src}
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src} ${deps}
                COMMENT "Building ISPC object ${outdir}/${fname}.dev${ISPC_TARGET_EXT}")

            set(ISPC_OBJECTS ${ISPC_OBJECTS} ${results})
            set(ISPC_HEADERS ${ISPC_HEADERS} ${outdirh}/${fname}_ispc.h)
        endforeach()
        add_custom_target(${ISPC_TARGET} DEPENDS ${ISPC_OBJECTS} ${ISPC_HEADERS})

    endmacro()

    macro(add_ispc_executable name)
        set(ISPC_SOURCES "")
        set(ISPC_HEADERS "")
        set(OTHER_SOURCES "")
        foreach(src ${ARGN})
            get_filename_component(ext ${src} EXT)
            if(ext STREQUAL ".ispc")
                set(ISPC_SOURCES ${ISPC_SOURCES} ${src})
            else()
                set(OTHER_SOURCES ${OTHER_SOURCES} ${src})
            endif()
        endforeach()
        ispc_compile(_ispc_exec_${name} ${ISPC_SOURCES})
        add_executable(${name} ${ISPC_OBJECTS} ${OTHER_SOURCES})
        add_dependencies(${name} _ispc_exec_${name})

        if(NOT __XEON__)
            foreach(src ${ISPC_OBJECTS})
                set_source_files_properties(${src} PROPERTIES COMPILE_FLAGS -std=gnu++98)
            endforeach()
        endif()
    endmacro()

    macro(add_ispc_library name type)
        set(ISPC_SOURCES "")
        set(ISPC_HEADERS "")
        set(OTHER_SOURCES "")
        foreach(src ${ARGN})
            get_filename_component(ext ${src} EXT)
            if(ext STREQUAL ".ispc")
                set(ISPC_SOURCES ${ISPC_SOURCES} ${src})
            else()
                set(OTHER_SOURCES ${OTHER_SOURCES} ${src})
            endif()
        endforeach()
        ispc_compile(_ispc_lib_${name} ${ISPC_SOURCES})
        add_library(${name} ${type} ${ISPC_OBJECTS} ${OTHER_SOURCES})
        add_dependencies(${name} _ispc_lib_${name})

        if(NOT __XEON__)
            foreach(src ${ISPC_OBJECTS})
                set_source_files_properties(${src} PROPERTIES COMPILE_FLAGS -std=gnu++98)
            endforeach()
        endif()
    endmacro()

else(ENABLE_ISPC_SUPPORT)

    macro(add_ispc_library name type)
        set(ISPC_SOURCES "")
        set(OTHER_SOURCES "")
        foreach(src ${ARGN})
            get_filename_component(ext ${src} EXT)
            if(ext STREQUAL ".ispc")
                set(ISPC_SOURCES ${ISPC_SOURCES} ${src})
            else()
                set(OTHER_SOURCES ${OTHER_SOURCES} ${src})
            endif()
        endforeach()
        add_library(${name} ${type} ${OTHER_SOURCES})

    endmacro()

endif(ENABLE_ISPC_SUPPORT)
