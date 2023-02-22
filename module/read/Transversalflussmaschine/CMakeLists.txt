add_module(Transversalflussmaschine "read a line or row from a csv file" Transversalflussmaschine.cpp Transversalflussmaschine.h)
target_include_directories(Transversalflussmaschine PRIVATE rapidcsv/src)
