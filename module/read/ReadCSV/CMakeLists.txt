set(HEADERS ReadCSV.h)

set(SOURCES ReadCSV.cpp)

add_module(ReadCSV "read .CSV tables" ${SOURCES} ${HEADERS})
