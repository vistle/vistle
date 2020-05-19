set(SENSEI_SOURCES
  rectilinerGrid.cpp
  sensei.cpp
  structuredGrid.cpp
  unstructuredGrid.cpp
)

set(SENSEI_HEADER
  exeption.h
  metaData.h
  rectilinerGrid.h
  sensei.h
  structuredGrid.h
  unstructuredGrid.h
)

if (VISTLE_SHM_PER_RANK)
   add_definitions(-DSHMPERRANK)
endif()

vistle_add_library(vistle_sensei ${SENSEI_SOURCES} ${SENSEI_HEADER})
include(GenerateExportHeader)
GENERATE_EXPORT_HEADER( vistle_sensei
             BASE_NAME V_INSITU_SENSEI
             EXPORT_MACRO_NAME V_INSITU_SENSEI_EXPORT
             EXPORT_FILE_NAME vistle_sensei_Export.h
             STATIC_DEFINE vistle_sensei_BUILT_AS_STATIC
)
target_include_directories(vistle_sensei PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(vistle_sensei
		PRIVATE vistle_module
		PRIVATE vistle_util
		PRIVATE vistle_control
        PRIVATE vistle_core
        PRIVATE vistle_boost_mpi
		PRIVATE vistle_insitu_message
		PUBLIC vistle_insitu_core
		PRIVATE ${Boost_LIBRARIES}
		PRIVATE ${MPI_C_LIBRARIES}
		)