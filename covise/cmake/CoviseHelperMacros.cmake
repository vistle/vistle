# @file CoviseHelperMacros.cmake
#
# @author Blasius Czink
#
# Provides helper macros for build control:
#
#  ADD_COVISE_LIBRARY(targetname [STATIC | SHARED | MODULE] [EXCLUDE_FROM_ALL]
#              source1 source2 ... sourceN)
#    - covise specific wrapper macro of add_library. Please use this macro for covise libraries.
#
#  ADD_COVISE_EXECUTABLE(targetname [WIN32] [MACOSX_BUNDLE] [EXCLUDE_FROM_ALL]
#              source1 source2 ... sourceN)
#    - covise specific wrapper macro of add_executable. Please use this macro for covise executables.
#      Note: The variables SOURCES and HEADERS are added automatically.
#
#  ADD_COVISE_MODULE category targetname [WIN32] [MACOSX_BUNDLE] [EXCLUDE_FROM_ALL]
#              source1 source2 ... sourceN)
#    - covise specific wrapper macro of add_executable. Please use this macro for covise modules.
#      You should specify the category of the module. Passing an empty string for category will have the same effect
#      as using ADD_COVISE_EXECUTABLE()
#      Note: The variables SOURCES and HEADERS are added automatically.
#
#  COVISE_INSTALL_TARGET(targetname)
#    - covise specific wrapper macro of INSTALL(TARGETS ...) Please use this macro for installing covise targets.
#      You should use the cmake INSTALL() command for anything else but covise-targets.
#
#  ADD_COVISE_COMPILE_FLAGS(targetname flags)
#    - add additional compile_flags to the given target
#      Example: ADD_COVISE_COMPILE_FLAGS(coJS "-fPIC;-fno-strict-aliasing")
#
#  REMOVE_COVISE_COMPILE_FLAGS(targetname flags)
#    - remove compile flags from target
#      Example: REMOVE_COVISE_COMPILE_FLAGS(coJS "-fPIC")
#
#  ADD_COVISE_LINK_FLAGS(targetname flags)
#    - add additional link flags to the given target
#
#  REMOVE_COVISE_LINK_FLAGS(targetname flags)
#    - remove link flags from target
#
#  COVISE_WERROR(targetname)
#    - convenience macro to add "warnings-are-errors" flag to a given target
#      You may pass additional parameters after targetname...
#      Example: COVISE_WERROR(coJS WIN32)  -  will switch target "coJS" to "warnings-are-errors" on all windows versions
#               COVISE_WERROR(coJS BASEARCH yoroo zackel) - will switch target "coJS" to "warnings-are-errors" on
#                                                           yoroo, yorooopt, zackel and zackelopt 
#
#  COVISE_WNOERROR(targetname)
#    - convenience macro to remove "warnings-are-errors" flag from a given target
#      (syntax is equivalent to the above macro)
#
#  COVISE_NOT_AVAILABLE_ON(where [...])
#    - convenience macro to disable configuration/compilation of certain parts of covise
#      Use this macro at the beginning of a CMakeLists.txt file. You may specify WIN32, UNIX, APPLE for parameter "where"
#      or you specify BASEARCH an then a list of basearchsuffixes.
#
#  COVISE_AVAILABLE_ON(where)
#    - opposite of COVISE_NOT_AVAILABLE_ON macro.
#
#  --------------------------------------------------------------------------------------
#
#  COVISE_ADJUST_LIB_VARS (basename)
#    - this is a support macro for cmake find modules (it simply sets/adjusts library-variables
#      depending on what was previously found for example by FIND_LIBRARY()
#
#  COVISE_RESET (varname)
#    - sets a variable to "not found"
#
#  COVISE_SET_FTPARAM (env_var_name env_var_value)
#    - set temporary environment variables for COVISE_TEST_FEATURE macro
#      (in case of libraries the debug versions will get filtered out)
#      Example: COVISE_SET_FTPARAM(EXAMPLE_FT_LIB "${MPICH_LIBRARIES}")
#
#  COVISE_TEST_FEATURE (feature_dest_var feature_test_name my_output)
#    - Compiles a small program given in "feature_test_name" and sets the variable "feature_dest_var"
#      if the compile/link process was successful
#      The full output from the compile/link process is returned in "my_output"
#      This macro expects the "feature-test"-files in CM_FEATURE_TESTS_DIR which is preset to
#      ${COVISEDIR}/cmake/FeatureTests
#
#      Example: COVISE_TEST_FEATURE(MPI_THREADED ft_mpi_threaded.c MY_OUTPUT)
#
#  COVISE_COPY_TARGET_PDB(target_name pdb_inst_prefix)
#    - gets the targets .pdb file and deploys it to the given location ${pdb_inst_prefix}/lib or
#      ${pdb_inst_prefix}/bin during install
#    - only the pdb files for debug versions of a library are installed
#      (this macro is windows specific)
# 
#  COVISE_DUMP_LIB_SETUP (basename)
#    - dumps the different library-variable contents to a file
#
#  USING(DEP1 DEP2 [optional])
#    - add dependencies DEP1 and DEP2,
#      all of them are optional, if 'optional' is present within the arguments
#
# @author Blasius Czink
#

message("covise helper macros")

# Macro to adjust library-variables
MACRO(COVISE_ADJUST_LIB_VARS basename)
  # if only the release version was found, set the debug variable also to the release version
  IF (${basename}_LIBRARY_RELEASE AND NOT ${basename}_LIBRARY_DEBUG)
    SET(${basename}_LIBRARY_DEBUG ${${basename}_LIBRARY_RELEASE} CACHE FILEPATH "${basename}_LIBRARY_DEBUG" FORCE)
    SET(${basename}_LIBRARY       ${${basename}_LIBRARY_RELEASE} CACHE FILEPATH "${basename}_LIBRARY" FORCE)
    SET(${basename}_LIBRARIES     ${${basename}_LIBRARY_RELEASE} CACHE FILEPATH "${basename}_LIBRARIES" FORCE)
  ENDIF (${basename}_LIBRARY_RELEASE AND NOT ${basename}_LIBRARY_DEBUG)

  # if only the debug version was found, set the release variable also to the debug version
  IF (${basename}_LIBRARY_DEBUG AND NOT ${basename}_LIBRARY_RELEASE)
    SET(${basename}_LIBRARY_RELEASE ${${basename}_LIBRARY_DEBUG} CACHE FILEPATH "${basename}_LIBRARY_RELEASE" FORCE)
    SET(${basename}_LIBRARY         ${${basename}_LIBRARY_DEBUG} CACHE FILEPATH "${basename}_LIBRARY" FORCE)
    SET(${basename}_LIBRARIES       ${${basename}_LIBRARY_DEBUG} CACHE FILEPATH "${basename}_LIBRARIES" FORCE)
  ENDIF (${basename}_LIBRARY_DEBUG AND NOT ${basename}_LIBRARY_RELEASE)
  
  # if both are set...
  IF (${basename}_LIBRARY_DEBUG AND ${basename}_LIBRARY_RELEASE)
    # if the generator supports configuration types then set
    # optimized and debug libraries, or if the CMAKE_BUILD_TYPE has a value
    IF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
      SET(${basename}_LIBRARY
          optimized ${${basename}_LIBRARY_RELEASE}
          debug     ${${basename}_LIBRARY_DEBUG}
      )
    ELSE(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
      # if there are no configuration types and CMAKE_BUILD_TYPE has no value
      # then just use the release libraries
      SET(${basename}_LIBRARY ${${basename}_LIBRARY_RELEASE} )
    ENDIF(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)

    SET(${basename}_LIBRARIES
        optimized ${${basename}_LIBRARY_RELEASE}
        debug ${${basename}_LIBRARY_DEBUG}
        CACHE FILEPATH "${basename}_LIBRARIES" FORCE
    )
    
  ENDIF (${basename}_LIBRARY_DEBUG AND ${basename}_LIBRARY_RELEASE)

  SET(${basename}_LIBRARY "${${basename}_LIBRARY}" CACHE FILEPATH "The ${basename} library" FORCE)
  # do not show those in normal view
  MARK_AS_ADVANCED(FORCE
    ${basename}_LIBRARY
    ${basename}_LIBRARY_RELEASE
    ${basename}_LIBRARY_DEBUG
    ${basename}_LIBRARIES
  )
ENDMACRO(COVISE_ADJUST_LIB_VARS)


# helper to reset variables to ...-NOTFOUND so that the FIND...-macros run again
MACRO(COVISE_RESET varname)
  SET (${varname} "${varname}-NOTFOUND")
ENDMACRO(COVISE_RESET)

# helper to dump the lib-values to a simple text-file
MACRO(COVISE_DUMP_LIB_SETUP basename)
  SET (dump_file "${CMAKE_BINARY_DIR}/${basename}_lib_setup.txt")
  FILE(WRITE  ${dump_file} "${basename}_INCLUDE_DIR    = ${${basename}_INCLUDE_DIR}\n")
  FILE(APPEND ${dump_file} "${basename}_LIBRARY        = ${${basename}_LIBRARY}\n")
  FILE(APPEND ${dump_file} "${basename}_LIBRARY_RELESE = ${${basename}_LIBRARY_RELEASE}\n")
  FILE(APPEND ${dump_file} "${basename}_LIBRARY_DEBUG  = ${${basename}_LIBRARY_DEBUG}\n")
  FILE(APPEND ${dump_file} "${basename}_LIBRARIES      = ${${basename}_LIBRARIES}\n")
ENDMACRO(COVISE_DUMP_LIB_SETUP)

# helper to print the lib-values to a simple text-file
MACRO(COVISE_PRINT_LIB_SETUP basename)
  MESSAGE("${basename}_INCLUDE_DIR    = ${${basename}_INCLUDE_DIR}")
  MESSAGE("${basename}_LIBRARY        = ${${basename}_LIBRARY}")
  MESSAGE("${basename}_LIBRARY_RELESE = ${${basename}_LIBRARY_RELEASE}")
  MESSAGE("${basename}_LIBRARY_DEBUG  = ${${basename}_LIBRARY_DEBUG}")
  MESSAGE("${basename}_LIBRARIES      = ${${basename}_LIBRARIES}")
ENDMACRO(COVISE_PRINT_LIB_SETUP)

MACRO(COVISE_SET_FTPARAM env_var_name env_var_value)
  SET(FOUT "")

  #MESSAGE("orig mystring is ${env_var_value}")

  # get rid of debug libs RegExp ([Dd][Ee][Bb][Uu][Gg];[^;]*;?)
  STRING (REGEX REPLACE "[Dd][Ee][Bb][Uu][Gg];[^;]*;?" "" FOUT "${env_var_value}")
  #MESSAGE("1.mystring is ${FOUT}")

  # get rid of "optimized"
  STRING (REPLACE "optimized;" "" FOUT "${FOUT}")
  #MESSAGE("2.mystring is ${FOUT}")

  # add backslash before ";"
  STRING (REPLACE ";" "\\;" FOUT "${FOUT}")
  #MESSAGE("3.mystring is ${FOUT}")

  SET(ENV{${env_var_name}} "${FOUT}")
ENDMACRO(COVISE_SET_FTPARAM)


MACRO(COVISE_CLEAN_DEBUG_LIBS var_name var_value)
  SET (FOUT "")
  # get rid of debug libs RegExp ([Dd][Ee][Bb][Uu][Gg];[^;]*;?)
  STRING (REGEX REPLACE "[Dd][Ee][Bb][Uu][Gg];[^;]*;?" "" FOUT "${var_value}")
  # get rid of "optimized"
  STRING (REPLACE "optimized;" "" FOUT "${FOUT}")
  # replace backslash with space
  STRING (REPLACE ";" " " FOUT "${FOUT}")
  SET(${var_name} "${FOUT}")
ENDMACRO(COVISE_CLEAN_DEBUG_LIBS)


MACRO(COVISE_TEST_FEATURE feature_dest_var feature_test_name my_output)
  MESSAGE (STATUS "Checking for ${feature_test_name}")
  TRY_COMPILE (${feature_dest_var}
               ${CMAKE_BINARY_DIR}/CMakeTemp
               ${CM_FEATURE_TESTS_DIR}/${feature_test_name}
               CMAKE_FLAGS
               -DINCLUDE_DIRECTORIES=$ENV{COVISE_FT_INC}
               -DLINK_LIBRARIES=$ENV{COVISE_FT_LIB}
               OUTPUT_VARIABLE ${my_output}
  )
  # Feature test failed
  IF (${feature_dest_var})
     MESSAGE (STATUS "Checking for ${feature_test_name} - succeeded")
  ELSE (${feature_dest_var})
     MESSAGE (STATUS "Checking for ${feature_test_name} - feature not available")
  ENDIF (${feature_dest_var})
ENDMACRO(COVISE_TEST_FEATURE)


FUNCTION(COVISE_COPY_TARGET_PDB target_name pdb_inst_prefix)
  IF(MSVC)
    GET_TARGET_PROPERTY(TARGET_LOC ${target_name} DEBUG_LOCATION)
    IF(TARGET_LOC)
      GET_FILENAME_COMPONENT(TARGET_HEAD "${TARGET_LOC}" NAME_WE)
      # GET_FILENAME_COMPONENT(FNABSOLUTE  "${TARGET_LOC}" ABSOLUTE)
      GET_FILENAME_COMPONENT(TARGET_PATH "${TARGET_LOC}" PATH)
      SET(TARGET_PDB "${TARGET_PATH}/${TARGET_HEAD}.pdb")
      # MESSAGE(STATUS "PDB-File of ${target_name} is ${TARGET_PDB}")
      GET_TARGET_PROPERTY(TARGET_TYPE ${target_name} TYPE)
      IF(TARGET_TYPE)
        SET(pdb_dest )
        IF(${TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
          SET(pdb_dest lib)
        ELSE(${TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
          SET(pdb_dest bin)
        ENDIF(${TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
        # maybe an optional category path given?
        IF(NOT ARGN STREQUAL "")
          SET(category_path "${ARGV2}")
        ENDIF(NOT ARGN STREQUAL "")
        INSTALL(FILES ${TARGET_PDB} DESTINATION "${pdb_inst_prefix}/${pdb_dest}${category_path}" CONFIGURATIONS "Debug") 
      ENDIF(TARGET_TYPE)
    ENDIF(TARGET_LOC)
  ENDIF(MSVC)
ENDFUNCTION(COVISE_COPY_TARGET_PDB)


MACRO(COVISE_INVERT_BOOL var)
  IF(${var})
    SET(${var} FALSE)
  ELSE(${var})
    SET(${var} TRUE)
  ENDIF(${var})
ENDMACRO(COVISE_INVERT_BOOL)


MACRO(COVISE_LIST_CONTAINS var value)
  SET(${var})
  FOREACH (value2 ${ARGN})
    STRING (TOUPPER ${value}  str1)
    STRING (TOUPPER ${value2} str2)
    IF (${str1} STREQUAL ${str2})
      SET (${var} TRUE)
    ENDIF (${str1} STREQUAL ${str2})
  ENDFOREACH (value2)
ENDMACRO(COVISE_LIST_CONTAINS)


MACRO(COVISE_LIST_CONTAINS_CS var value)
  SET (${var})
  FOREACH (value2 ${ARGN})
    IF (${value} STREQUAL ${value2})
      SET (${var} TRUE)
    ENDIF (${value} STREQUAL ${value2})
  ENDFOREACH (value2)
ENDMACRO(COVISE_LIST_CONTAINS_CS)


MACRO(COVISE_MSVC_RUNTIME_OPTION)
  IF(MSVC)
    OPTION (MSVC_USE_STATIC_RUNTIME "Use static MS-Runtime (/MT, /MTd)" OFF)
    IF(MSVC_USE_STATIC_RUNTIME)
      FOREACH(FLAG_VAR CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
                        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
      IF(${FLAG_VAR} MATCHES "/MD")
        STRING(REGEX REPLACE "/MD" "/MT" BCMSVC_${FLAG_VAR} "${${FLAG_VAR}}")
        SET(${FLAG_VAR} ${BCMSVC_${FLAG_VAR}} CACHE STRING "" FORCE)
      ENDIF(${FLAG_VAR} MATCHES "/MD")
      ENDFOREACH(FLAG_VAR)
    ELSE(MSVC_USE_STATIC_RUNTIME)
      FOREACH(FLAG_VAR CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
                        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
      IF(${FLAG_VAR} MATCHES "/MT")
        STRING(REGEX REPLACE "/MT" "/MD" BCMSVC_${FLAG_VAR} "${${FLAG_VAR}}")
        SET(${FLAG_VAR} ${BCMSVC_${FLAG_VAR}} CACHE STRING "" FORCE)
      ENDIF(${FLAG_VAR} MATCHES "/MT")
      ENDFOREACH(FLAG_VAR)
    ENDIF(MSVC_USE_STATIC_RUNTIME)
  ENDIF(MSVC)
ENDMACRO(COVISE_MSVC_RUNTIME_OPTION)

MACRO(COVISE_HAS_PREPROCESSOR_ENTRY CONTENTS KEYWORD VARIABLE)
  STRING(REGEX MATCH "\n *# *define +(${KEYWORD})" PREPROC_TEMP_VAR ${${CONTENTS}})
  
  IF (CMAKE_MATCH_1)
    SET(${VARIABLE} TRUE)
  ELSE (CMAKE_MATCH_1)
    set(${VARIABLE} FALSE)
  ENDIF (CMAKE_MATCH_1)
  
ENDMACRO(COVISE_HAS_PREPROCESSOR_ENTRY)

# Macro to adjust the output directories of a target
FUNCTION(COVISE_ADJUST_OUTPUT_DIR targetname)
  GET_TARGET_PROPERTY(TARGET_TYPE ${targetname} TYPE)
  IF(TARGET_TYPE)
    # MESSAGE("COVISE_ADJUST_OUTPUT_DIR(${targetname}) : TARGET_TYPE = ${TARGET_TYPE}")
    IF(TARGET_TYPE STREQUAL EXECUTABLE)
      SET(BINLIB_SUFFIX "bin")
    ELSE()
      SET(BINLIB_SUFFIX "lib")
    ENDIF()
    # optional binlib override
    IF(NOT ${ARGV2} STREQUAL "")
      SET(BINLIB_SUFFIX ${ARGV2})
    ENDIF()    

    SET(MYPATH_POSTFIX )
    # optional path-postfix specified?
    IF(NOT ${ARGV1} STREQUAL "")
      IF("${ARGV1}" MATCHES "^/.*")
        SET(MYPATH_POSTFIX "${ARGV1}")
      ELSE()
        SET(MYPATH_POSTFIX "/${ARGV1}")
      ENDIF()
    ENDIF()
    
    # adjust
    IF(CMAKE_CONFIGURATION_TYPES)
      # generator supports configuration types
      FOREACH(conf_type ${CMAKE_CONFIGURATION_TYPES})
        STRING(TOUPPER "${conf_type}" upper_conf_type_str)
        IF(COVISE_PLACE_BINARIES_INSOURCE)
          IF(upper_conf_type_str STREQUAL "DEBUG")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${upper_conf_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${upper_conf_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${upper_conf_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          ELSE(upper_conf_type_str STREQUAL "DEBUG")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${upper_conf_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${upper_conf_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${upper_conf_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          ENDIF(upper_conf_type_str STREQUAL "DEBUG")
        ELSE(COVISE_PLACE_BINARIES_INSOURCE)
          IF(upper_conf_type_str STREQUAL "DEBUG")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${upper_conf_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${upper_conf_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${upper_conf_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          ELSE(upper_conf_type_str STREQUAL "DEBUG")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${upper_conf_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${upper_conf_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
            SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${upper_conf_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          ENDIF(upper_conf_type_str STREQUAL "DEBUG")
        ENDIF(COVISE_PLACE_BINARIES_INSOURCE)
      ENDFOREACH(conf_type)
    ELSE(CMAKE_CONFIGURATION_TYPES)
      # no configuration types - probably makefile generator
      STRING(TOUPPER "${CMAKE_BUILD_TYPE}" upper_build_type_str)
      IF(COVISE_PLACE_BINARIES_INSOURCE)
        IF(upper_build_type_str STREQUAL "DEBUG")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${upper_build_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${upper_build_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${upper_build_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
        ELSE(upper_build_type_str STREQUAL "DEBUG")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${upper_build_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${upper_build_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${upper_build_type_str} "${COVISEDIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
        ENDIF(upper_build_type_str STREQUAL "DEBUG")
      ELSE(COVISE_PLACE_BINARIES_INSOURCE)
        IF(upper_build_type_str STREQUAL "DEBUG")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${upper_build_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${upper_build_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${upper_build_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
        ELSE(upper_build_type_str STREQUAL "DEBUG")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${upper_build_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${upper_build_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
          SET_TARGET_PROPERTIES(${ARGV0} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${upper_build_type_str} "${CMAKE_BINARY_DIR}/${DBG_ARCHSUFFIX}opt/${BINLIB_SUFFIX}${MYPATH_POSTFIX}")
        ENDIF(upper_build_type_str STREQUAL "DEBUG")
      ENDIF(COVISE_PLACE_BINARIES_INSOURCE)
    ENDIF(CMAKE_CONFIGURATION_TYPES)
    
  ELSE(TARGET_TYPE)
    MESSAGE("COVISE_ADJUST_OUTPUT_DIR() - Could not retrieve target type of ${targetname}")
  ENDIF(TARGET_TYPE)
ENDFUNCTION(COVISE_ADJUST_OUTPUT_DIR)

# Macro to add covise libraries
MACRO(ADD_COVISE_LIBRARY targetname)
  ADD_LIBRARY(${ARGV} ${SOURCES} ${HEADERS})
  # SET_TARGET_PROPERTIES(${targetname} PROPERTIES PROJECT_LABEL "${targetname}")
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES OUTPUT_NAME "${targetname}")
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES VERSION "${COVISE_MAJOR_VERSION}.${COVISE_MINOR_VERSION}")

  COVISE_ADJUST_OUTPUT_DIR(${targetname})
  
  # set additional COVISE_COMPILE_FLAGS
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES COMPILE_FLAGS "${COVISE_COMPILE_FLAGS}")
  # set additional COVISE_LINK_FLAGS
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES LINK_FLAGS "${COVISE_LINK_FLAGS}")
  
  #SET_TARGET_PROPERTIES(${targetname} PROPERTIES DEBUG_OUTPUT_NAME "${targetname}${CMAKE_DEBUG_POSTFIX}")
  #SET_TARGET_PROPERTIES(${targetname} PROPERTIES RELEASE_OUTPUT_NAME "${targetname}${CMAKE_RELEASE_POSTFIX}")
  #SET_TARGET_PROPERTIES(${targetname} PROPERTIES RELWITHDEBINFO_OUTPUT_NAME "${targetname}${CMAKE_RELWITHDEBINFO_POSTFIX}")
  #SET_TARGET_PROPERTIES(${targetname} PROPERTIES MINSIZEREL_OUTPUT_NAME "${targetname}${CMAKE_MINSIZEREL_POSTFIX}")
  
  UNSET(SOURCES)
  UNSET(HEADERS)
ENDMACRO(ADD_COVISE_LIBRARY)

# Macro to add covise executables
MACRO(ADD_COVISE_EXECUTABLE targetname)
  ADD_EXECUTABLE(${targetname} ${ARGN} ${SOURCES} ${HEADERS})
  # SET_TARGET_PROPERTIES(${targetname} PROPERTIES PROJECT_LABEL "${targetname}")
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES OUTPUT_NAME "${targetname}")
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES VERSION "${COVISE_MAJOR_VERSION}.${COVISE_MINOR_VERSION}")
  
  COVISE_ADJUST_OUTPUT_DIR(${targetname})
  
  # set additional COVISE_COMPILE_FLAGS
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES COMPILE_FLAGS "${COVISE_COMPILE_FLAGS}")
  # set additional COVISE_LINK_FLAGS
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES LINK_FLAGS "${COVISE_LINK_FLAGS}")
  
  # SET_TARGET_PROPERTIES(${targetname} PROPERTIES DEBUG_OUTPUT_NAME "${targetname}${CMAKE_DEBUG_POSTFIX}")
  UNSET(SOURCES)
  UNSET(HEADERS)
ENDMACRO(ADD_COVISE_EXECUTABLE)

# Macro to add covise modules (executables with a module-category)
MACRO(ADD_COVISE_MODULE category targetname)
  ADD_EXECUTABLE(${targetname} ${ARGN} ${SOURCES} ${HEADERS})
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES OUTPUT_NAME "${targetname}")
  # SET_TARGET_PROPERTIES(${targetname} PROPERTIES VERSION "${COVISE_MAJOR_VERSION}.${COVISE_MINOR_VERSION}")
  
  COVISE_ADJUST_OUTPUT_DIR(${targetname} ${category})
  
  # set additional COVISE_COMPILE_FLAGS
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES COMPILE_FLAGS "${COVISE_COMPILE_FLAGS}")
  # set additional COVISE_LINK_FLAGS
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES LINK_FLAGS "${COVISE_LINK_FLAGS}")
  # use the LABELS property on the target to save the category
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES LABELS "${category}")
  
  # SET_TARGET_PROPERTIES(${targetname} PROPERTIES DEBUG_OUTPUT_NAME "${targetname}${CMAKE_DEBUG_POSTFIX}")
  UNSET(SOURCES)
  UNSET(HEADERS)
ENDMACRO(ADD_COVISE_MODULE)


MACRO(ADD_COVER_PLUGIN targetname)
  ADD_COVER_PLUGIN_TARGET(${targetname})
  IF(${ARGC} GREATER 1 OR DEFINED EXTRA_LIBS)
    TARGET_LINK_LIBRARIES(${targetname} ${ARGN} ${EXTRA_LIBS})
  ENDIF()
  COVER_INSTALL_PLUGIN(${targetname})
ENDMACRO(ADD_COVER_PLUGIN)

# Macro to add OpenCOVER plugins
MACRO(ADD_COVER_PLUGIN_TARGET targetname)
  IF(NOT OPENSCENEGRAPH_FOUND)
    RETURN()
  ENDIF()
  USING(VTK optional)
  # setup what we need from Qt
  SET(QT_DONT_USE_QTGUI 1)
  INCLUDE(${QT_USE_FILE})
  
  IF(WIN32)
    ADD_DEFINITIONS(-DIMPORT_PLUGIN)
  ENDIF(WIN32)

  INCLUDE_DIRECTORIES(
    ${ZLIB_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR}
    ${PNG_INCLUDE_DIR}
    ${TIFF_INCLUDE_DIR}
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
    "${COVISEDIR}/src/kernel/OpenVRUI"
    "${COVISEDIR}/src/renderer/OpenCOVER"
  )

  ADD_LIBRARY(${targetname} SHARED ${ARGN} ${SOURCES} ${HEADERS})
  
  IF(APPLE)
    ADD_COVISE_LINK_FLAGS(${targetname} "-undefined error")
  ENDIF(APPLE)
  
  # SET_TARGET_PROPERTIES(${targetname} PROPERTIES PROJECT_LABEL "${targetname}")
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES OUTPUT_NAME "${targetname}")
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES VERSION "${COVISE_MAJOR_VERSION}.${COVISE_MINOR_VERSION}")

  COVISE_ADJUST_OUTPUT_DIR(${targetname} "OpenCOVER/plugins")
  
  # set additional COVISE_COMPILE_FLAGS
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES COMPILE_FLAGS "${COVISE_COMPILE_FLAGS}")
  # set additional COVISE_LINK_FLAGS
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES LINK_FLAGS "${COVISE_LINK_FLAGS}")
  # swith off "lib" prefix for MinGW
  IF(MINGW)
    SET_TARGET_PROPERTIES(${targetname} PROPERTIES PREFIX "")
  ENDIF(MINGW)
    
  TARGET_LINK_LIBRARIES(${targetname} coOpenPluginUtil coOpenCOVER coOpenVRUI coOSGVRUI coVRBClient coConfig coUtil ${OPENSCENEGRAPH_LIBRARIES}) # ${CMAKE_THREAD_LIBS_INIT})
  
  UNSET(SOURCES)
  UNSET(HEADERS)
ENDMACRO(ADD_COVER_PLUGIN_TARGET)

MACRO(COVISE_INSTALL_DEPENDENCIES targetname)
  IF (${targetname}_LIB_DEPENDS)
     IF(WIN32)
	 SET(upper_build_type_str "RELEASE")
     ELSE(WIN32)
        STRING(TOUPPER ${CMAKE_BUILD_TYPE} upper_build_type_str)
	 ENDIF(WIN32)
     # Get dependencies
     SET(depends "${targetname}_LIB_DEPENDS")
#    MESSAGE(${${depends}})
     LIST(LENGTH ${depends} len)
     MATH(EXPR len "${len} - 2")
#     MESSAGE(${len})
     FOREACH(ctr RANGE 0 ${len} 2)
       # Split dependencies in mode (optimized, debug, general) and library name
       LIST(GET ${depends} ${ctr} mode)      
       MATH(EXPR ctr "${ctr} + 1")
       LIST(GET ${depends} ${ctr} value)
       STRING(TOUPPER ${mode} mode)
       # Check if the library is required for the current build type
       IF(${mode} STREQUAL "GENERAL")
         SET(check_install "1")
       ELSEIF(${mode} STREQUAL ${upper_build_type_str})
         SET(check_install "1")
       ELSE(${mode} STREQUAL "GENERAL")
         SET(check_install "0")
       ENDIF(${mode} STREQUAL "GENERAL")
       IF(${check_install})
         # If the library is from externlibs, pack it. 
         # FIXME: Currently only works with libraries added by FindXXX, as manually added 
         # libraries usually lack the full path.
         IF (value MATCHES "^$ENV{EXTERNLIBS}")
#           MESSAGE("VALUE+ ${value}")
           IF (IS_DIRECTORY ${value})
           ELSE (IS_DIRECTORY ${value})
           INSTALL(FILES ${value} DESTINATION covise/${ARCHSUFFIX}/lib)
           # Recurse symlinks
           # FIXME: Does not work with absolute links (that are evil anyway)
           WHILE(IS_SYMLINK ${value})
             EXECUTE_PROCESS(COMMAND readlink -n ${value} OUTPUT_VARIABLE link_target)
             GET_FILENAME_COMPONENT(link_dir ${value} PATH)
             SET(value "${link_dir}/${link_target}")
#             MESSAGE("VALUE ${value}")
             INSTALL(FILES ${value} DESTINATION covise/${ARCHSUFFIX}/lib)
           ENDWHILE(IS_SYMLINK ${value})
           ENDIF (IS_DIRECTORY ${value})
         ENDIF (value MATCHES "^$ENV{EXTERNLIBS}")
       ENDIF(${check_install})
     ENDFOREACH(ctr)
  ENDIF (${targetname}_LIB_DEPENDS)
ENDMACRO(COVISE_INSTALL_DEPENDENCIES)


# Macro to install and export
MACRO(COVISE_INSTALL_TARGET targetname)
  # was a category specified for the given target?
  GET_TARGET_PROPERTY(category ${targetname} LABELS)
  SET(_category_path )
  IF(category)
    SET(_category_path "/${category}")
  ENDIF(category)
  
  # @Florian: What are you trying to do? The following will create a subdirectory "covise/${ARCHSUFFIX}/..."
  # in each and every in-source subdirectory where you issue a COVISE_INSTALL_TARGET() !!!
  # cmake's INSTALL() will create the given subdirectories in ${CMAKE_INSTALL_PREFIX} at install time.
  #
#  IF (NOT EXISTS covise/${ARCHSUFFIX}/bin)
#    FILE(MAKE_DIRECTORY covise/${ARCHSUFFIX}/bin)
#  ENDIF(NOT EXISTS covise/${ARCHSUFFIX}/bin)
#  IF (NOT EXISTS covise/${ARCHSUFFIX}/bin${_category_path})
#    FILE(MAKE_DIRECTORY covise/${ARCHSUFFIX}/bin${_category_path})
#  ENDIF(NOT EXISTS covise/${ARCHSUFFIX}/bin${_category_path})
#  IF (NOT EXISTS covise/${ARCHSUFFIX}/lib)
#    FILE(MAKE_DIRECTORY covise/${ARCHSUFFIX}/lib)
#  ENDIF(NOT EXISTS covise/${ARCHSUFFIX}/lib)

  INSTALL(TARGETS ${ARGV} EXPORT covise-targets
          RUNTIME DESTINATION covise/${ARCHSUFFIX}/bin${_category_path}
          LIBRARY DESTINATION covise/${ARCHSUFFIX}/lib
          ARCHIVE DESTINATION covise/${ARCHSUFFIX}/lib
          COMPONENT modules.${category}
  )
  IF(COVISE_EXPORT_TO_INSTALL)
    INSTALL(EXPORT covise-targets DESTINATION covise/${ARCHSUFFIX}/lib COMPONENT modules.${category})
  ELSE(COVISE_EXPORT_TO_INSTALL)
    # EXPORT(TARGETS ${ARGV} APPEND FILE "${CMAKE_BINARY_DIR}/${BASEARCHSUFFIX}/covise-exports.cmake")
    EXPORT(TARGETS ${ARGV} APPEND FILE "${COVISEDIR}/${ARCHSUFFIX}/covise-exports.cmake")
  ENDIF(COVISE_EXPORT_TO_INSTALL)
  FOREACH(tgt ${ARGV})
    COVISE_COPY_TARGET_PDB(${tgt} covise/${ARCHSUFFIX} ${_category_path})
  ENDFOREACH(tgt)
  COVISE_INSTALL_DEPENDENCIES(${targetname})
ENDMACRO(COVISE_INSTALL_TARGET)

# Macro to install an OpenCOVER plugin
MACRO(COVER_INSTALL_PLUGIN targetname)
  INSTALL(TARGETS ${ARGV} EXPORT covise-targets
          RUNTIME DESTINATION covise/${ARCHSUFFIX}/lib/OpenCOVER/plugins
          LIBRARY DESTINATION covise/${ARCHSUFFIX}/lib/OpenCOVER/plugins
          ARCHIVE DESTINATION covise/${ARCHSUFFIX}/lib/OpenCOVER/plugins
          COMPONENT osgplugins.${category}
  )
  COVISE_INSTALL_DEPENDENCIES(${targetname})
ENDMACRO(COVER_INSTALL_PLUGIN)

#
# Per target flag handling
#

FUNCTION(ADD_COVISE_COMPILE_FLAGS targetname flags)
  GET_TARGET_PROPERTY(MY_CFLAGS ${targetname} COMPILE_FLAGS)
  IF(NOT MY_CFLAGS)
    SET(MY_CFLAGS "")
  ENDIF()
  FOREACH(cflag ${flags})
    #STRING(REGEX MATCH "${cflag}" flag_matched "${MY_CFLAGS}")
    #IF(NOT flag_matched)
      SET(MY_CFLAGS "${MY_CFLAGS} ${cflag}")
    #ENDIF(NOT flag_matched)
  ENDFOREACH(cflag)
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES COMPILE_FLAGS "${MY_CFLAGS}")
  # MESSAGE("added compile flags ${MY_CFLAGS} to target ${targetname}")
ENDFUNCTION(ADD_COVISE_COMPILE_FLAGS)

FUNCTION(REMOVE_COVISE_COMPILE_FLAGS targetname flags)
  GET_TARGET_PROPERTY(MY_CFLAGS ${targetname} COMPILE_FLAGS)
  IF(NOT MY_CFLAGS)
    SET(MY_CFLAGS "")
  ENDIF()
  FOREACH(cflag ${flags})
    STRING(REGEX REPLACE "${cflag}[ ]+|${cflag}$" "" MY_CFLAGS "${MY_CFLAGS}")
  ENDFOREACH(cflag)
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES COMPILE_FLAGS "${MY_CFLAGS}")
ENDFUNCTION(REMOVE_COVISE_COMPILE_FLAGS)

FUNCTION(ADD_COVISE_LINK_FLAGS targetname flags)
  GET_TARGET_PROPERTY(MY_LFLAGS ${targetname} LINK_FLAGS)
  IF(NOT MY_LFLAGS)
    SET(MY_LFLAGS "")
  ENDIF()
  FOREACH(lflag ${flags})
    #STRING(REGEX MATCH "${lflag}" flag_matched "${MY_LFLAGS}")
    #IF(NOT flag_matched)
      SET(MY_LFLAGS "${MY_LFLAGS} ${lflag}")
    #ENDIF(NOT flag_matched)
  ENDFOREACH(lflag)
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES LINK_FLAGS "${MY_LFLAGS}")
  # MESSAGE("added link flags ${MY_LFLAGS} to target ${targetname}")
ENDFUNCTION(ADD_COVISE_LINK_FLAGS)

FUNCTION(REMOVE_COVISE_LINK_FLAGS targetname flags)
  GET_TARGET_PROPERTY(MY_LFLAGS ${targetname} LINK_FLAGS)
  IF(NOT MY_LFLAGS)
    SET(MY_LFLAGS "")
  ENDIF()
  FOREACH(lflag ${flags})
    STRING(REGEX REPLACE "${lflag}[ ]+|${lflag}$" "" MY_LFLAGS "${MY_LFLAGS}")
  ENDFOREACH(lflag)
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES LINK_FLAGS "${MY_LFLAGS}")
ENDFUNCTION(REMOVE_COVISE_LINK_FLAGS)

# small debug helper
FUNCTION(OUTPUT_COVISE_COMPILE_FLAGS targetname)
  GET_TARGET_PROPERTY(MY_CFLAGS ${targetname} COMPILE_FLAGS)
  MESSAGE("Target ${targetname} - COMPILE_FLAGS = ${MY_CFLAGS}")
ENDFUNCTION(OUTPUT_COVISE_COMPILE_FLAGS)

FUNCTION(OUTPUT_COVISE_LINK_FLAGS targetname)
  GET_TARGET_PROPERTY(MY_LFLAGS ${targetname} LINK_FLAGS)
  MESSAGE("Target ${targetname} - LINK_FLAGS = ${MY_LFLAGS}")
ENDFUNCTION(OUTPUT_COVISE_LINK_FLAGS)

# Set per target warnings-are-errors flag
FUNCTION(COVISE_WERROR targetname)
  # any archsuffixes or system names like (WIN32, UNIX, APPLE, MINGW etc.) passed as optional params?
  IF(${ARGC} GREATER 1)
    # we have optional stuff
    IF("${ARGV1}" STREQUAL "BASEARCH")
      # we expect BASEARCHSUFFIXES in the following params
      FOREACH(arch ${ARGN})
        IF("${arch}" STREQUAL "${BASEARCHSUFFIX}")
          ADD_COVISE_COMPILE_FLAGS(${targetname} "${COVISE_WERROR_FLAG}")
        ENDIF("${arch}" STREQUAL "${BASEARCHSUFFIX}")
      ENDFOREACH(arch)
    ELSE("${ARGV1}" STREQUAL "BASEARCH")
      # we expect ONE additional param like WIN32, UNIX, APPLE, MSVC, MINGW etc.
      IF(${ARGV1})
        ADD_COVISE_COMPILE_FLAGS(${targetname} "${COVISE_WERROR_FLAG}")
      ENDIF(${ARGV1})
    ENDIF("${ARGV1}" STREQUAL "BASEARCH")
  ELSE(${ARGC} GREATER 1)
    # only target is given
    ADD_COVISE_COMPILE_FLAGS(${targetname} "${COVISE_WERROR_FLAG}")
  ENDIF(${ARGC} GREATER 1)
ENDFUNCTION(COVISE_WERROR)

# Remove per target warnings-are-errors flag
FUNCTION(COVISE_WNOERROR targetname)
  # any archsuffixes or system names like (WIN32, UNIX, APPLE, MINGW etc.) passed as optional params?
  IF(${ARGC} GREATER 1)
    # we have optional stuff
    IF("${ARGV1}" STREQUAL "BASEARCH")
      # we expect BASEARCHSUFFIXES in the following params
      FOREACH(arch ${ARGN})
        IF("${arch}" STREQUAL "${BASEARCHSUFFIX}")
          REMOVE_COVISE_COMPILE_FLAGS(${targetname} "${COVISE_WERROR_FLAG}")
        ENDIF("${arch}" STREQUAL "${BASEARCHSUFFIX}")
      ENDFOREACH(arch)
    ELSE("${ARGV1}" STREQUAL "BASEARCH")
      # we expect ONE additional param like WIN32, UNIX, APPLE, MSVC, MINGW etc.
      IF(${ARGV1})
        REMOVE_COVISE_COMPILE_FLAGS(${targetname} "${COVISE_WERROR_FLAG}")
      ENDIF(${ARGV1})
    ENDIF("${ARGV1}" STREQUAL "BASEARCH")
  ELSE(${ARGC} GREATER 1)
    # only target is given
    REMOVE_COVISE_COMPILE_FLAGS(${targetname} "${COVISE_WERROR_FLAG}")
  ENDIF(${ARGC} GREATER 1)
ENDFUNCTION(COVISE_WNOERROR)

MACRO(COVISE_NOT_AVAILABLE_ON where)
  GET_FILENAME_COMPONENT(mycurrent_dirname "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
  IF("${ARGV0}" STREQUAL "BASEARCH")
    # we got a list of basearchsuffixes
    FOREACH(arch ${ARGN})
      IF("${arch}" STREQUAL "${BASEARCHSUFFIX}")
        MESSAGE("Skipping ${mycurrent_dirname} since it is marked as NOT AVALABLE ON ${BASEARCHSUFFIX}.")
        RETURN()
      ENDIF("${arch}" STREQUAL "${BASEARCHSUFFIX}")
    ENDFOREACH(arch)
  ELSE("${ARGV0}" STREQUAL "BASEARCH")
    # we got a cmake system variable like WIN32, UNIX, MINGW, etc.
    IF(${where})
      MESSAGE("Skipping ${mycurrent_dirname} since it is marked as NOT AVALABLE ON ${where}.")
      RETURN()
    ENDIF(${where})
  ENDIF("${ARGV0}" STREQUAL "BASEARCH")
ENDMACRO(COVISE_NOT_AVAILABLE_ON)

MACRO(COVISE_AVAILABLE_ON where)
  GET_FILENAME_COMPONENT(mycurrent_dirname "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
  IF("${ARGV0}" STREQUAL "BASEARCH")
    # we got a list of basearchsuffixes
    SET(my_list "${ARGN}")
    LIST(FIND my_list "${BASEARCHSUFFIX}" item_idx)
    IF("${item_idx}" EQUAL -1)
      MESSAGE("Skipping ${mycurrent_dirname} since it is marked as AVALABLE ON ${ARGN} only.")
      RETURN()
    ENDIF("${item_idx}" EQUAL -1)
  ELSE("${ARGV0}" STREQUAL "BASEARCH")
    # we got a cmake system variable like WIN32, UNIX, MINGW, etc.
    IF(NOT ${where})
      MESSAGE("Skipping ${mycurrent_dirname} since it is marked as AVALABLE ON ${where} only.")
      RETURN()
    ENDIF(NOT ${where})
  ENDIF("${ARGV0}" STREQUAL "BASEARCH")
ENDMACRO(COVISE_AVAILABLE_ON)

MACRO(COVISE_UNFINISHED)
  STRING(REPLACE "${CMAKE_SOURCE_DIR}/" "" MYDIR "${CMAKE_CURRENT_SOURCE_DIR}")
  MESSAGE("Warning: Skipping unfinished CMakeLists.txt in ${MYDIR}")
  RETURN()
ENDMACRO(COVISE_UNFINISHED)

# ----------------
#  Unused stuff
# ----------------

# MACRO(COVISE_FILTER_OUT FILTERS INPUTS OUTPUT)
#   # Arguments:
#   #  FILTERS - list of patterns that need to be removed
#   #  INPUTS  - list of inputs that will be worked on
#   #  OUTPUT  - the filtered list to be returned
#   # 
#   # Example: 
#   #  SET(MYLIST this that and the other)
#   #  SET(FILTS this that)
#   #
#   #  FILTER_OUT("${FILTS}" "${MYLIST}" OUT)
#   #  MESSAGE("OUTPUT = ${OUT}")
#   #
#   # The output - 
#   #   OUTPUT = and;the;other
#   #
#   SET(FOUT "")
#   FOREACH(INP ${INPUTS})
#      SET(FILTERED 0)
#      FOREACH(FILT ${FILTERS})
#          IF(${FILTERED} EQUAL 0)
#              IF("${FILT}" STREQUAL "${INP}")
#                  SET(FILTERED 1)
#              ENDIF("${FILT}" STREQUAL "${INP}")
#          ENDIF(${FILTERED} EQUAL 0)
#      ENDFOREACH(FILT ${FILTERS})
#      IF(${FILTERED} EQUAL 0)
#          SET(FOUT ${FOUT} ${INP})
#      ENDIF(${FILTERED} EQUAL 0)
#   ENDFOREACH(INP ${INPUTS})
#   SET(${OUTPUT} ${FOUT})
# ENDMACRO(COVISE_FILTER_OUT)

# MACRO (CHECK_LIBRARY LIB_NAME LIB_DESC LIB_TEST_SOURCE)
#   SET (HAVE_${LIB_NAME})
#   MESSAGE (STATUS "Checking for ${LIB_DESC} library...")
#   TRY_COMPILE (HAVE_${LIB_NAME} ${CMAKE_BINARY_DIR}/.cmake_temp ${CMAKE_SOURCE_DIR}/cmake/${LIB_TEST_SOURCE})
#   IF (HAVE_${LIB_NAME})
#      # Don't need one
#      MESSAGE(STATUS "Checking for ${LIB_DESC} library... none needed")
#   ELSE (HAVE_${LIB_NAME})
#      # Try to find a suitable library
#      FOREACH (lib ${ARGN})
#          TRY_COMPILE (HAVE_${LIB_NAME} ${CMAKE_BINARY_DIR}/.cmake_temp
#                      ${CMAKE_SOURCE_DIR}/cmake/${LIB_TEST_SOURCE}
#                      CMAKE_FLAGS -DLINK_LIBRARIES:STRING=${lib})
#          IF (HAVE_${LIB_NAME})
#            MESSAGE (STATUS "Checking for ${LIB_DESC} library... ${lib}")
#            SET (HAVE_${LIB_NAME}_LIB ${lib})
#          ENDIF (HAVE_${LIB_NAME})
#      ENDFOREACH (lib)
#   ENDIF (HAVE_${LIB_NAME})
#   # Unable to find a suitable library
#   IF (NOT HAVE_${LIB_NAME})
#      MESSAGE (STATUS "Checking for ${LIB_DESC} library... not found")
#   ENDIF (NOT HAVE_${LIB_NAME})
# ENDMACRO (CHECK_LIBRARY)
# 
# ...the LIB_NAME param controls the name of the variable(s) that are set 
# if it finds the library. LIB_DESC is used for the status messages. An 
# example usage might be:
# 
# CHECK_LIBRARY(SCK socket scktest.c socket xnet ws2_32)


# Macro to link with COVISE and other libraries, COVISE libraries get their dependencies pulled in automatically
MACRO(COVISE_TARGET_LINK_LIBRARIES targetname libs)
   set(COUNT 0)
   foreach(A ${ARGV})
      if(NOT ${COUNT} EQUAL 0)
         if("${A}" STREQUAL "coAlg")
	    USE_VTK(OPTIONAL)
         endif("${A}" STREQUAL "coAlg")
      endif(NOT ${COUNT} EQUAL 0)
      math(EXPR COUNT "${COUNT}+1")
   endforeach(A ${ARGV})
   TARGET_LINK_LIBRARIES(${ARGV})
ENDMACRO(COVISE_TARGET_LINK_LIBRARIES)

MACRO(TESTIT)
ENDMACRO(TESTIT)

MACRO(USE_INVENTOR)
  MESSAGE("inventor:")
  MESSAGE(INTENTOR_FOUND)
  IF(NOT INVENTOR_FOUND)
  ENDIF(NOT INVENTOR_FOUND)

  IF ((NOT INVENTOR_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing Inventor")
    RETURN()
  ENDIF((NOT INVENTOR_FOUND) AND (${ARGC} LESS 1))
  IF(NOT INVENTOR_USED AND INVENTOR_FOUND)
    SET(INVENTOR_USED TRUE)  
    INCLUDE_DIRECTORIES(${INVENTOR_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${INVENTOR_XT_LIBRARY} ${INVENTOR_IMAGE_LIBRARY} ${INVENTOR_LIBRARY} ${INVENTOR_FL_LIBRARY} ${JPEG_LIBRARY})
    IF(INVENTOR_FREETYPE_LIBRARY)
      SET(EXTRA_LIBS ${EXTRA_LIBS} ${INVENTOR_FREETYPE_LIBRARY})
    ENDIF()
  ENDIF()
ENDMACRO(USE_INVENTOR)

MACRO(USE_VTK)
  IF ((NOT VTK_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing VTK")
    RETURN()
  ENDIF((NOT VTK_FOUND) AND (${ARGC} LESS 1))
  IF(NOT VTK_USED AND VTK_FOUND)
    SET(VTK_USED TRUE)
    IF(MSVC)
      SET(VTK_LIBRARIESIO debug vtkFilteringd optimized vtkFiltering debug vtkDICOMParserd optimized vtkDICOMParser debug vtkNetCDFd optimized vtkNetCDF debug vtkmetaiod optimized vtkmetaio debug vtksqlited optimized vtksqlite debug vtkpngd optimized vtkpng debug vtkzlibd optimized vtkzlib debug vtkjpegd optimized vtkjpeg debug vtktiffd optimized vtktiff debug vtkexpatd optimized vtkexpat debug vtksysd optimized vtksys general vfw32)
      SET(VTK_LIBRARIES debug vtkRenderingd optimized vtkRendering debug vtkGraphicsd optimized vtkGraphics debug vtkImagingd optimized vtkImaging debug vtkIOd optimized vtkIO debug vtkftgld optimized vtkftgl debug vtkfreetyped optimized vtkfreetype general opengl32)
      SET(VTK_LIBRARIESFILTERING debug vtkCommond optimized vtkCommon)
      SET(VTK_LIBRARIESCOMMON debug vtksysd optimized vtksys)
    
      LINK_DIRECTORIES(${VTK_LIBRARY_DIRS})
      INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIRS})
      ADD_DEFINITIONS(-DHAVE_VTK)
      SET(EXTRA_LIBS ${EXTRA_LIBS} ${VTK_LIBRARIES} ${VTK_LIBRARIESIO} ${VTK_LIBRARIESFILTERING} ${VTK_LIBRARIESCOMMON})
    ELSE(MSVC)
      INCLUDE(${VTK_USE_FILE})
# atismer: works to find vtk-5.8 in opensuse 12.1
##testing
#      LINK_DIRECTORIES(${VTK_LIBRARY_DIRS})
#      INCLUDE(${VTK_CMAKE_DIR}/vtkMakeInstantiator.cmake)
##
      IF(MINGW)
          SET(EXTRA_LIBS ${EXTRA_LIBS} vtkCommon vtkGraphics vtkIO vtkFiltering)
      ELSE(MINGW)
          SET(EXTRA_LIBS ${EXTRA_LIBS} vtkCommon vtkGraphics vtkIO vtkRendering vtkFiltering)
      ENDIF(MINGW)
      ADD_DEFINITIONS(-DHAVE_VTK)
      IF(CMAKE_COMPILER_IS_GNUXX OR APPLE)
         SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
      ENDIF()
    ENDIF(MSVC)
  ENDIF()
ENDMACRO(USE_VTK)

MACRO(USE_OPENGL)
  IF ((NOT OPENGL_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing OpenGL")
    RETURN()
  ENDIF((NOT OPENGL_FOUND) AND (${ARGC} LESS 1))
  IF(NOT OPENGL_USED AND OPENGL_FOUND)
    SET(OPENGL_USED TRUE)
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${OPENGL_LIBRARIES})
  ENDIF()
ENDMACRO(USE_OPENGL)

MACRO(USE_BOOST)
  FIND_PACKAGE(BOOST)
  IF ((NOT BOOST_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing BOOST")
    RETURN()
  ENDIF((NOT BOOST_FOUND) AND (${ARGC} LESS 1))
  IF(NOT BOOST_USED AND BOOST_FOUND)
    SET(BOOST_USED TRUE)
    INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${BOOST_LIBRARIES})
  ENDIF()
ENDMACRO(USE_BOOST)

MACRO(USE_OSC)
  FIND_PACKAGE(OSC)
  IF ((NOT OSC_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing OSC")
    RETURN()
  ENDIF((NOT OSC_FOUND) AND (${ARGC} LESS 1))
  IF(NOT OSC_USED AND OSC_FOUND)
    SET(OSC_USED TRUE)
    INCLUDE_DIRECTORIES(${OSC_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${OSC_LIBRARIES})
  ENDIF()
ENDMACRO(USE_OSC)

MACRO(USE_ZLIB)
  IF ((NOT ZLIB_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing ZLIB")
    RETURN()
  ENDIF((NOT ZLIB_FOUND) AND (${ARGC} LESS 1))
  IF(NOT ZLIB_USED AND ZLIB_FOUND)
    SET(ZLIB_USED TRUE)
    INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${ZLIB_LIBRARIES})
  ENDIF()
ENDMACRO(USE_ZLIB)

MACRO(USE_STEEREO)
  IF ((NOT STEEREO_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing STEEREO")
    RETURN()
  ENDIF((NOT STEEREO_FOUND) AND (${ARGC} LESS 1))
  IF(NOT STEEREO_USED AND STEEREO_FOUND)
    SET(STEEREO_USED TRUE)
    INCLUDE_DIRECTORIES(${STEEREO_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${STEEREO_LIBRARIES})
  ENDIF()
ENDMACRO(USE_STEEREO)

MACRO(USE_PNG)
  IF ((NOT PNG_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing PNG")
    RETURN()
  ENDIF((NOT PNG_FOUND) AND (${ARGC} LESS 1))
  IF(NOT PNG_USED AND PNG_FOUND)
    SET(PNG_USED TRUE)
    INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${PNG_LIBRARIES})
  ENDIF()
ENDMACRO(USE_PNG)

MACRO(USE_ABAQUS)
  FIND_PACKAGE(Abaqus)
  IF ((NOT ABAQUS_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing ABAQUS")
    RETURN()
  ENDIF((NOT ABAQUS_FOUND) AND (${ARGC} LESS 1))
  IF(NOT ABAQUS_USED AND ABAQUS_FOUND)
    SET(ABAQUS_USED TRUE)
    INCLUDE_DIRECTORIES(${ABAQUS_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${ABAQUS_LIBRARIES})
  ENDIF()
ENDMACRO(USE_ABAQUS)

MACRO(USE_GLUT)
  IF ((NOT GLUT_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing GLUT")
    RETURN()
  ENDIF((NOT GLUT_FOUND) AND (${ARGC} LESS 1))
  IF(NOT GLUT_USED AND GLUT_FOUND)
    SET(GLUT_USED TRUE)
    USE_OPENGL(optional)
    ADD_DEFINITIONS(-D GLUT_NO_LIB_PRAGMA)  
    
    SET(CUDA_NVCC_DEFINITIONS ${CUDA_NVCC_DEFINITIONS} GLUT_NO_LIB_PRAGMA)
    INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${GLUT_LIBRARIES})
    IF(UNIX AND NOT APPLE)
      SET(EXTRA_LIBS ${EXTRA_LIBS} Xxf86vm)
    ENDIF()
  ENDIF()
ENDMACRO(USE_GLUT)

MACRO(USE_TCL)
  FIND_PACKAGE(TCL)
  IF ((NOT TCL_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing TCL")
    RETURN()
  ENDIF((NOT TCL_FOUND) AND (${ARGC} LESS 1))
  IF(NOT TCL_USED AND TCL_FOUND)
    SET(TCL_USED TRUE)
    INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${TCL_LIBRARIES})
  ENDIF()
ENDMACRO(USE_TCL)

MACRO(USE_OSSIMPLANET)
  FIND_PACKAGE(OSSIMPLANET)
  IF ((NOT OSSIMPLANET_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing OSSIMPLANET")
    RETURN()
  ENDIF((NOT OSSIMPLANET_FOUND) AND (${ARGC} LESS 1))
  IF(NOT OSSIMPLANET_USED AND OSSIMPLANET_FOUND)
    SET(OSSIMPLANET_USED TRUE)
    INCLUDE_DIRECTORIES(${OSSIMPLANET_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${OSSIMPLANET_LIBRARIES})
  ENDIF()
ENDMACRO(USE_OSSIMPLANET)

MACRO(USE_CAVEUI)
  IF(NOT CAVEUI_USED)
    SET(CAVEUI_USED TRUE)
    #INCLUDE_DIRECTORIES(${CAVEUI_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} osgcaveui)
  ENDIF(NOT CAVEUI_USED)
ENDMACRO(USE_CAVEUI)

MACRO(USE_VIRVO)
  IF(NOT VIRVO_USED)
    SET(VIRVO_USED TRUE)
    ADD_DEFINITIONS(-DVV_APPLICATION_BUILD)
    #INCLUDE_DIRECTORIES(${VIRVO_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} coVirvo)
  ENDIF(NOT VIRVO_USED)
ENDMACRO(USE_VIRVO)

 
MACRO(USE_XERCESC)
  IF ((NOT XERCESC_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing XERCESC")
    RETURN()
  ENDIF((NOT XERCESC_FOUND) AND (${ARGC} LESS 1))
  IF(NOT XERCESC_USED AND XERCESC_FOUND)
    SET(XERCESC_USED TRUE)
    INCLUDE_DIRECTORIES(${XERCESC_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${XERCESC_LIBRARIES})
  ENDIF()
ENDMACRO(USE_XERCESC)

MACRO(USE_GLEW)
  IF ((NOT GLEW_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing GLEW")
    RETURN()
  ENDIF((NOT GLEW_FOUND) AND (${ARGC} LESS 1))
  IF(NOT GLEW_USED AND GLEW_FOUND)
    SET(GLEW_USED TRUE)
    USE_OPENGL()
    INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${GLEW_LIBRARIES})
  ENDIF()
ENDMACRO(USE_GLEW)
  
MACRO(USE_MOTIF)
  IF ((NOT MOTIF_FOUND OR NOT X11_Xt_FOUND OR NOT X11_Xinput_FOUND OR NOT X11_Xext_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing Motif")
    RETURN()
  ENDIF ((NOT MOTIF_FOUND OR NOT X11_Xt_FOUND OR NOT X11_Xinput_FOUND OR NOT X11_Xext_FOUND) AND (${ARGC} LESS 1))
  IF(NOT MOTIF_USED AND MOTIF_FOUND)
    SET(MOTIF_USED TRUE)
    INCLUDE_DIRECTORIES(${X11_Xt_INCLUDE_PATH} ${MOTIF_INCLUDE_DIR} )
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${MOTIF_LIBRARIES} ${X11_X11_LIB}  ${X11_Xt_LIB} ${X11_Xinput_LIB} ${X11_Xext_LIB})
  ENDIF()
ENDMACRO(USE_MOTIF)  

MACRO(USE_CG)
  IF ((NOT CG_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing CG")
    RETURN()
  ENDIF((NOT CG_FOUND) AND (${ARGC} LESS 1))
  IF(NOT CG_USED AND CG_FOUND)
    SET(CG_USED TRUE)
    USE_OPENGL()
    ADD_DEFINITIONS(-DHAVE_CG)
    INCLUDE_DIRECTORIES(${CG_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${CG_LIBRARY} ${CG_GL_LIBRARY})
  ENDIF()
ENDMACRO(USE_CG)

MACRO(USE_CUDPP)
  IF ((NOT CUDPP_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing CUDPP")
    RETURN()
  ENDIF((NOT CUDPP_FOUND) AND (${ARGC} LESS 1))
  IF(NOT CUDPP_USED AND CUDPP_FOUND)
    SET(CUDPP_USED TRUE)
    INCLUDE_DIRECTORIES(${CUDPP_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${CUDPP_LIBRARIES})
  ENDIF()
ENDMACRO(USE_CUDPP)

MACRO(USE_ARTOOLKITPLUS)
  FIND_PACKAGE(ARTOOLKITPLUS)
  IF ((NOT ARTOOLKITPLUS_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing ARTOOLKITPLUS")
    RETURN()
  ENDIF((NOT ARTOOLKITPLUS_FOUND) AND (${ARGC} LESS 1))
  IF(NOT ARTOOLKITPLUS_USED AND ARTOOLKITPLUS_FOUND)
    SET(ARTOOLKITPLUS_USED TRUE)
    ADD_DEFINITIONS(-DHAVE_AR)
    INCLUDE_DIRECTORIES(${ARTOOLKITPLUS_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${ARTOOLKITPLUS_LIBRARIES})
  ENDIF()
ENDMACRO(USE_ARTOOLKITPLUS)

MACRO(USE_ARTOOLKIT)
  IF ((NOT ARTOOLKIT_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing ARTOOLKIT")
    RETURN()
  ENDIF((NOT ARTOOLKIT_FOUND) AND (${ARGC} LESS 1))
  IF(NOT ARTOOLKIT_USED AND ARTOOLKIT_FOUND)
    SET(ARTOOLKIT_USED TRUE)
    ADD_DEFINITIONS(-DHAVE_AR)
    INCLUDE_DIRECTORIES(${ARTOOLKIT_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${ARTOOLKIT_LIBRARIES})
  ENDIF()
ENDMACRO(USE_ARTOOLKIT)

MACRO(USE_CAL3D)
  FIND_PACKAGE(CAL3D)
  IF ((NOT CAL3D_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing CAL3D")
    RETURN()
  ENDIF((NOT CAL3D_FOUND) AND (${ARGC} LESS 1))
  IF(NOT CAL3D_USED AND CAL3D_FOUND)
    SET(CAL3D_USED TRUE)
    INCLUDE_DIRECTORIES(${CAL3D_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${CAL3D_LIBRARIES})
  ENDIF()
ENDMACRO(USE_CAL3D)

MACRO(USE_OSGCAL)
  FIND_PACKAGE(OSGCAL)
  IF ((NOT OSGCAL_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing OSGCAL")
    RETURN()
  ENDIF((NOT OSGCAL_FOUND) AND (${ARGC} LESS 1))
  IF(NOT OSGCAL_USED AND OSGCAL_FOUND)
    SET(OSGCAL_USED TRUE)
    USE_CAL3D()
    INCLUDE_DIRECTORIES(${OSGCAL_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${OSGCAL_LIBRARIES})
  ENDIF()
ENDMACRO(USE_OSGCAL)

MACRO(USE_OSGQT)
  FIND_PACKAGE(OsgQt)
  IF ((NOT OSGQT_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing OSGQT")
    RETURN()
  ENDIF((NOT OSGQT_FOUND) AND (${ARGC} LESS 1))
  IF(NOT OSGQT_USED AND OSGQT_FOUND)
    SET(OSGQT_USED TRUE)
    INCLUDE_DIRECTORIES(${OSGQT_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${OSGQT_LIBRARIES})
  ENDIF()
ENDMACRO(USE_OSGQT)

MACRO(USE_CGNS)
  FIND_PACKAGE(CGNS)
  IF ((NOT CGNS_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing CGNS")
    RETURN()
  ENDIF((NOT CGNS_FOUND) AND (${ARGC} LESS 1))
  IF(NOT CGNS_USED AND CGNS_FOUND)
    SET(CGNS_USED TRUE)
    USE_OPENGL()
    INCLUDE_DIRECTORIES(${CGNS_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${CGNS_LIBRARIES})
  ENDIF()
ENDMACRO(USE_CGNS)

MACRO(USE_OSGEARTH)
  FIND_PACKAGE(OsgEarth)
  IF (((NOT OSGEARTH_FOUND) OR (NOT OSGEARTHANNOTATION_LIBRARY)) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing OSGEARTH")
    RETURN()
  ENDIF(((NOT OSGEARTH_FOUND) OR (NOT OSGEARTHANNOTATION_LIBRARY)) AND (${ARGC} LESS 1))
  IF(NOT OSGEARTH_USED AND OSGEARTH_FOUND)
    SET(OSGEARTH_USED TRUE)
    USE_OPENGL()
    INCLUDE_DIRECTORIES(${OSGEARTH_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${OSGEARTH_LIBRARIES})
  ENDIF()
ENDMACRO(USE_OSGEARTH)

MACRO(USE_WIIYOURSELF)
  FIND_PACKAGE(WiiYourself)
  IF ((NOT WIIYOURSELF_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing WIIYOURSELF")
    RETURN()
  ENDIF((NOT WIIYOURSELF_FOUND) AND (${ARGC} LESS 1))
  IF(NOT WIIYOURSELF_USED AND WIIYOURSELF_FOUND)
    SET(WIIYOURSELF_USED TRUE)
    INCLUDE_DIRECTORIES(${WIIYOURSELF_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${WIIYOURSELF_LIBRARIES})
  ENDIF()
ENDMACRO(USE_WIIYOURSELF)

MACRO(USE_SIXENSE)
  FIND_PACKAGE(Sixense)
  IF ((NOT SIXENSE_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing SIXENSE")
    RETURN()
  ENDIF((NOT SIXENSE_FOUND) AND (${ARGC} LESS 1))
  IF(NOT SIXENSE_USED AND SIXENSE_FOUND)
    SET(SIXENSE_USED TRUE)
    INCLUDE_DIRECTORIES(${SIXENSE_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${SIXENSE_LIBRARIES})
  ENDIF()
ENDMACRO(USE_SIXENSE)

MACRO(USE_TIFF)
  FIND_PACKAGE(TIFF)
  IF ((NOT TIFF_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing TIFF")
    RETURN()
  ENDIF((NOT TIFF_FOUND) AND (${ARGC} LESS 1))
  IF(NOT TIFF_USED AND TIFF_FOUND)
    SET(TIFF_USED TRUE)
    INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${TIFF_LIBRARIES})
  ENDIF()
ENDMACRO(USE_TIFF)

MACRO(USE_MPI)
  FIND_PACKAGE(MPI)
  IF ((NOT MPI_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing MPI")
    RETURN()
  ENDIF((NOT MPI_FOUND) AND (${ARGC} LESS 1))
  IF(NOT MPI_USED AND MPI_FOUND)
    SET(MPI_USED TRUE)
    ADD_DEFINITIONS(-DHAS_MPI)
    INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${MPI_LIBRARIES})
  ENDIF()
ENDMACRO(USE_MPI)

MACRO(USE_OSGEPHEMERIS)
  FIND_PACKAGE(OsgEphemeris)
  IF ((NOT OSGEPHEMERIS_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing OSGEPHEMERIS")
    RETURN()
  ENDIF((NOT OSGEPHEMERIS_FOUND) AND (${ARGC} LESS 1))
  IF(NOT OSGEPHEMERIS_USED AND OSGEPHEMERIS_FOUND)
    SET(OSGEPHEMERIS_USED TRUE)
    USE_OPENGL()
    INCLUDE_DIRECTORIES(${OSGEPHEMERIS_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${OSGEPHEMERIS_LIBRARY})
  ENDIF()
ENDMACRO(USE_OSGEPHEMERIS)

MACRO(USE_GSOAP)
  IF ((NOT GSOAP_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing GSOAP")
    RETURN()
  ENDIF((NOT GSOAP_FOUND) AND (${ARGC} LESS 1))
  IF(NOT GSOAP_USED AND GSOAP_FOUND)
    SET(GSOAP_USED TRUE)
    ADD_DEFINITIONS(-DHAVE_GSOAP)
    INCLUDE_DIRECTORIES(${GSOAP_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${GSOAP_LIBRARY})
  ENDIF()
ENDMACRO(USE_GSOAP)

MACRO(USE_JT)
  FIND_PACKAGE(JT)
  IF ((NOT JT_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing JT")
    RETURN()
  ENDIF((NOT JT_FOUND) AND (${ARGC} LESS 1))
  IF(NOT JT_USED AND JT_FOUND)
    SET(JT_USED TRUE)
    INCLUDE_DIRECTORIES(${JT_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${JT_LIBRARIES})
  ENDIF()
ENDMACRO(USE_JT)

MACRO(USE_HDF5)
  FIND_PACKAGE(HDF5)
  IF ((NOT HDF5_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing HDF5")
    RETURN()
  ENDIF((NOT HDF5_FOUND) AND (${ARGC} LESS 1))
  IF(NOT HDF5_USED AND HDF_FOUND)
    SET(HDF5_USED TRUE)
    INCLUDE_DIRECTORIES(${HDF5_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${HDF5_LIBRARIES})
  ENDIF()
ENDMACRO(USE_HDF5)

MACRO(USE_NETCDF)
  FIND_PACKAGE(NetCDF)
  USE_HDF5(optional)
  IF ((NOT NETCDF_FOUND) AND (${ARGC} LESS 1))
    MESSAGE("Skipping because of missing NETCDF")
    RETURN()
  ENDIF((NOT NETCDF_FOUND) AND (${ARGC} LESS 1))
  IF(NOT NETCDF_USED AND NETCDF_FOUND)
    SET(NETCDF_USED TRUE)
    INCLUDE_DIRECTORIES(${NETCDF_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${NETCDF_LIBRARIES} ${NETCDF_C++_LIBRARY})
  ENDIF()
ENDMACRO(USE_NETCDF)


MACRO(USE_ITK)
SET(ITK_FIND_QUIETLY TRUE)
SET(ITK_FIND_REQUIRED FALSE)
COVISE_FIND_PACKAGE(ITK)
IF(ITK_FOUND)
  INCLUDE(${ITK_USE_FILE})
  IF(WIN32)
	#IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		SET(ITK_LIBRARIES_DEBUG )
		SET(ITK_LIBRARIES_TMP "${ITK_LIBRARIES}")
		SET(ITK_LIBRARIES_COMPLETE )
		WHILE(ITK_LIBRARIES_TMP)
		    LIST(GET ITK_LIBRARIES_TMP 0 ITK_LIBRARY)
			IF (${ITK_LIBRARY}_LIB_DEPENDS)
				LIST(APPEND ITK_LIBRARIES_COMPLETE ${${ITK_LIBRARY}_LIB_DEPENDS})
				LIST(APPEND ITK_LIBRARIES_TMP ${${ITK_LIBRARY}_LIB_DEPENDS})
			ENDIF (${ITK_LIBRARY}_LIB_DEPENDS)
			LIST(REMOVE_AT ITK_LIBRARIES_TMP 0)
		ENDWHILE(ITK_LIBRARIES_TMP)
		
		FOREACH(ITK_LIBRARY ${ITK_LIBRARIES_COMPLETE})
			STRING(REGEX REPLACE "[iI][tT][kK](.*)"
					"itk\\1d" ITK_LIBRARY_DEBUG
					"${ITK_LIBRARY}")
			LIST(APPEND ITK_LIBRARIES_DEBUG optimized ${ITK_LIBRARY} debug ${ITK_LIBRARY_DEBUG})
		ENDFOREACH(ITK_LIBRARY)
		SET(ITK_LIBRARIES "${ITK_LIBRARIES_DEBUG}")
	#ENDIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  ENDIF(WIN32)
ELSE(ITK_FOUND)
  MESSAGE("Skipping since ITK was not found! Please set ITK_DIR if you have it.")
  RETURN()
ENDIF(ITK_FOUND)
ENDMACRO(USE_ITK)

MACRO(CREATE_USING)
   message("create using")
  FIND_PROGRAM(GREP_EXECUTABLE grep PATHS $ENV{EXTERNLIBS}/UnixUtils DOC "grep executable")
  
  EXECUTE_PROCESS(COMMAND ${GREP_EXECUTABLE} USE_ "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CoviseHelperMacros.cmake"
                  COMMAND ${GREP_EXECUTABLE} "^MACRO"
                  OUTPUT_VARIABLE using_list)
		  #message("using_list")
                  #message("using_list ${using_list}")

  STRING(STRIP "${using_list}" using_list)

  STRING(REGEX REPLACE "MACRO\\([^_]*_\([^\n]*\)\\)" "\\1" using_list "${using_list}")
  STRING(REGEX REPLACE " optional" "" using_list "${using_list}")
  STRING(REGEX REPLACE "\n" ";" using_list "${using_list}")

  MESSAGE("USING LIST: ${using_list}")

  SET(filename "${CMAKE_BINARY_DIR}/cmake/CoviseUsingMacros.cmake")
  LIST(LENGTH using_list using_list_size)
  MATH(EXPR using_list_size "${using_list_size} - 1")

  FILE(WRITE  ${filename} "MACRO(USING)\n\n")
  FILE(APPEND ${filename} "  SET(optional FALSE)\n")
  FILE(APPEND ${filename} "  STRING (REGEX MATCHALL \"(^|[^a-zA-Z0-9_])optional(\$|[^a-zA-Z0-9_])\" optional \"\$")
  FILE(APPEND ${filename} "{ARGV}\")\n\n")
  FILE(APPEND ${filename} "  FOREACH(use \$")
  FILE(APPEND ${filename} "{ARGV})\n\n")
  FILE(APPEND ${filename} "    STRING (TOUPPER \${use} use)\n")

  FOREACH(ctr RANGE ${using_list_size})
    LIST(GET using_list ${ctr} target_macro)
    FILE(APPEND ${filename} "    IF(use STREQUAL ${target_macro})\n")
    FILE(APPEND ${filename} "      USE_${target_macro}(\${optional})\n")
    FILE(APPEND ${filename} "    ENDIF(use STREQUAL ${target_macro})\n\n")
  ENDFOREACH(ctr)

  FILE(APPEND ${filename} "  ENDFOREACH(use)\n\n")
  FILE(APPEND ${filename} "ENDMACRO(USING)\n")

  INCLUDE(${filename})

ENDMACRO(CREATE_USING)

# use this instead of FIND_PACKAGE to prefer Package in $PACKAGE_HOME and $EXTERNLIBS/package
MACRO(COVISE_FIND_PACKAGE package)
   SET(SAVED_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})

   STRING(TOUPPER ${package} UPPER)
   STRING(TOLOWER ${package} LOWER)
   IF(MINGW)
      SET(CMAKE_PREFIX_PATH ${MINGW_SYSROOT} ${CMAKE_PREFIX_PATH})
   ENDIF()
   IF(NOT "$ENV{EXTERNLIBS}" STREQUAL "")
      SET(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS}/${LOWER}/bin ${CMAKE_PREFIX_PATH})
      SET(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS} ${CMAKE_PREFIX_PATH})
      SET(CMAKE_PREFIX_PATH $ENV{EXTERNLIBS}/${LOWER} ${CMAKE_PREFIX_PATH})
   ENDIF()
   IF(NOT "$ENV{${UPPER}_HOME}" STREQUAL "")
      SET(CMAKE_PREFIX_PATH $ENV{${UPPER}_HOME} ${CMAKE_PREFIX_PATH})
   ENDIF()
   #message("looking for package ${ARGV}")
   #message("CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
   FIND_PACKAGE(${ARGV})

   SET(CMAKE_PREFIX_PATH ${SAVED_CMAKE_PREFIX_PATH})
ENDMACRO(COVISE_FIND_PACKAGE PACKAGE)
