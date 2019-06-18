set(HEADERS
  Element.h
  ReadDyna3D.h
)

set(SOURCES
  Element.cpp
  ReadDyna3D.cpp
)

add_module(ReadDyna3D ${SOURCES} ${HEADERS})
