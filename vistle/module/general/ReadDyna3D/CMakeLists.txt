set(HEADERS
  Element.h
  ReadDyna3D.h
  Dyna3DReader.h
)

set(SOURCES
  Element.cpp
  ReadDyna3D.cpp
  Dyna3DReader.cpp
)

add_module(ReadDyna3D ${SOURCES} ${HEADERS})
