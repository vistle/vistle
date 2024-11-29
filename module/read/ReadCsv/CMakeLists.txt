set(HEADERS ReadCsv.h)

set(SOURCES ReadCsv.cpp)

add_module(ReadCsv "read .CSV tables" ${SOURCES} ${HEADERS})
