set(HEADERS OpenFile.h ReadNek5000.h PartitionReader.h ReaderBase.h)

set(SOURCES OpenFile.cpp ReadNek5000.cpp PartitionReader.cpp ReaderBase.cpp)

add_module(ReadNek5000 "read .nek5000 files" ${SOURCES} ${HEADERS})
