add_module(WriteVistle WriteVistle.cpp)
target_link_libraries(WriteVistle "write Vistle" ${Boost_SERIALIZATION_LIBRARY})
