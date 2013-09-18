# - Find COVER

function(covise_find_library module library)

   find_library(${module}_LIBRARY
       NAMES ${library}
       PATH_SUFFIXES lib
       PATHS
       $ENV{COVISEDIR}/$ENV{ARCHSUFFIX}
       DOC "${module} - Library"
   )

endfunction()

find_package(COVISE)

if(COVER_INCLUDE_DIR)
   #set(COVER_FIND_QUIETLY TRUE)
endif()

find_path(COVER_INCLUDE_DIR "kernel/coVRPluginSupport.h"
   PATHS
   $ENV{COVISEDIR}/src/renderer/OpenCOVER
   DOC "COVER - Headers"
)

covise_find_library(COVER coOpenCOVER)
covise_find_library(PLUGINUTIL coOpenPluginUtil)
covise_find_library(VRUI coOpenVRUI)
covise_find_library(OSGVRUI coOSGVRUI)
covise_find_library(VRBCLIENT coVRBClient)
covise_find_library(COCONFIG coConfig)
covise_find_library(COUTIL coUtil)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(COVER DEFAULT_MSG
   COVER_LIBRARY PLUGINUTIL_LIBRARY VRUI_LIBRARY OSGVRUI_LIBRARY VRBCLIENT_LIBRARY COCONFIG_LIBRARY COUTIL_LIBRARY
   COVER_INCLUDE_DIR COVISE_INCLUDE_DIR)
mark_as_advanced(COVER_LIBRARY COVER_INCLUDE_DIR)

if(COVER_FOUND)
   set(COVER_INCLUDE_DIRS ${COVER_INCLUDE_DIR} ${COVISE_INCLUDE_DIR})
endif()

set(COVISE_PLACE_BINARIES_INSOURCE TRUE)

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
  #USING(VTK optional)
  
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

  set(LIBRARY_OUTPUT_PATH "${LIBRARY_OUTPUT_PATH}/OpenCOVER/plugins")
  ADD_LIBRARY(${targetname} SHARED ${ARGN} ${SOURCES} ${HEADERS})
  
  set_target_properties(${targetname} PROPERTIES FOLDER "OpenCOVER_Plugins")
  
  IF(APPLE)
    ADD_COVISE_LINK_FLAGS(${targetname} "-undefined error")
  ENDIF(APPLE)
  
  # SET_TARGET_PROPERTIES(${targetname} PROPERTIES PROJECT_LABEL "${targetname}")
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES OUTPUT_NAME "${targetname}")
  IF(NOT MINGW)
    SET_TARGET_PROPERTIES(${targetname} PROPERTIES VERSION "${COVISE_MAJOR_VERSION}.${COVISE_MINOR_VERSION}")
  ENDIF()

  #COVISE_ADJUST_OUTPUT_DIR(${targetname} "OpenCOVER/plugins")
  
  # set additional COVISE_COMPILE_FLAGS
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES COMPILE_FLAGS "${COVISE_COMPILE_FLAGS}")
  # set additional COVISE_LINK_FLAGS
  SET_TARGET_PROPERTIES(${targetname} PROPERTIES LINK_FLAGS "${COVISE_LINK_FLAGS}")
  # switch off "lib" prefix for MinGW
  IF(MINGW)
    SET_TARGET_PROPERTIES(${targetname} PROPERTIES PREFIX "")
  ENDIF(MINGW)
    
  TARGET_LINK_LIBRARIES(${targetname} ${PLUGINUTIL_LIBRARY} ${COVER_LIBRARY} ${VRUI_LIBRARY} ${OSGVRUI_LIBRARY} ${VRBCLIENT_LIBRARY} ${COCONFIG_LIBRARY} ${COUTIL_LIBRARY} ${OPENSCENEGRAPH_LIBRARIES}) # ${CMAKE_THREAD_LIBS_INIT})
  
  UNSET(SOURCES)
  UNSET(HEADERS)

  qt_use_modules(${targetname} Core)
ENDMACRO(ADD_COVER_PLUGIN_TARGET)

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

MACRO(COVISE_INSTALL_DEPENDENCIES targetname)
  IF (${targetname}_LIB_DEPENDS)
     IF(WIN32)
	 SET(upper_build_type_str "RELEASE")
     ELSE(WIN32)
        STRING(TOUPPER "${CMAKE_BUILD_TYPE}" upper_build_type_str)
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
         string(REPLACE "++" "\\+\\+" extlibs $ENV{EXTERNLIBS})
         IF (value MATCHES "^${extlibs}")
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
         ENDIF (value MATCHES "^${extlibs}")
       ENDIF(${check_install})
     ENDFOREACH(ctr)
  ENDIF (${targetname}_LIB_DEPENDS)
ENDMACRO(COVISE_INSTALL_DEPENDENCIES)

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

