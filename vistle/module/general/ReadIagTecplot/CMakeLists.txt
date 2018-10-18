set(HEADERS
   ReadIagTecplot.h
   mesh.h
   tecplotfile.h
   sources.h
   topo.h
)

set(SOURCES
   ReadIagTecplot.cpp
   mesh.cpp
   tecplotfile.cpp
)

add_module(ReadIagTecplot ${HEADERS} ${SOURCES})

target_link_libraries(ReadIagTecplot
   ${VTK_LIBRARIES}
)

