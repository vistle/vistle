# @file CoviseHelperMacros.cmake
#
# @author Blasius Czink
#
# Provides helper macros for build control:
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
#  USING(DEP1 DEP2 [optional])
#    - add dependencies DEP1 and DEP2,
#      all of them are optional, if 'optional' is present within the arguments
#
# @author Blasius Czink
#

FUNCTION(COVISE_WERROR targetname)
ENDFUNCTION(COVISE_WERROR)

# Remove per target warnings-are-errors flag
FUNCTION(COVISE_WNOERROR targetname)
ENDFUNCTION(COVISE_WNOERROR)


MACRO(ADD_COVER_PLUGIN targetname)
  ADD_COVER_PLUGIN_TARGET(${targetname})
  IF(${ARGC} GREATER 1 OR DEFINED EXTRA_LIBS)
    TARGET_LINK_LIBRARIES(${targetname} ${ARGN} ${EXTRA_LIBS})
  ENDIF()
  #COVER_INSTALL_PLUGIN(${targetname})
ENDMACRO(ADD_COVER_PLUGIN)

# Macro to add OpenCOVER plugins
MACRO(ADD_COVER_PLUGIN_TARGET targetname)
  IF(NOT OPENSCENEGRAPH_FOUND)
    RETURN()
  ENDIF()
  
  add_definitions(-DIMPORT_PLUGIN)

  set(QT_DONT_USE_QTGUI 1)
  include(${QT_USE_FILE})

  include_directories(
    ${ZLIB_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR}
    ${PNG_INCLUDE_DIR}
    ${TIFF_INCLUDE_DIR}
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
  )

  set(LIBRARY_OUTPUT_PATH "${LIBRARY_OUTPUT_PATH}/OpenCOVER/plugins")
  ADD_LIBRARY(${targetname} SHARED ${ARGN} ${SOURCES} ${HEADERS})
  
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES OUTPUT_NAME "${targetname}")

  # switch off "lib" prefix for MinGW
  IF(MINGW)
    SET_TARGET_PROPERTIES(${targetname} PROPERTIES PREFIX "")
  ENDIF(MINGW)
    
  TARGET_LINK_LIBRARIES(${targetname} covise_pluginutil covise_cover covise_osgvrui covise_vrbclient covise_config covise_util ${OPENSCENEGRAPH_LIBRARIES}) # ${CMAKE_THREAD_LIBS_INIT})
  
  UNSET(SOURCES)
  UNSET(HEADERS)
ENDMACRO(ADD_COVER_PLUGIN_TARGET)


MACRO(CREATE_USING)
  FIND_PROGRAM(GREP_EXECUTABLE grep PATHS $ENV{EXTERNLIBS}/UnixUtils DOC "grep executable")
  
  file(GLOB USING_FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Using/Use*.cmake"})
  foreach(F ${USING_FILES})
     include(${F})
  endforeach(F ${USING_FILES})
  EXECUTE_PROCESS(COMMAND ${GREP_EXECUTABLE} -h USE_ ${USING_FILES}
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
