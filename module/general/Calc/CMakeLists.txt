add_module(Calc "compute with coordinates and data" Calc.cpp Calc.h)
if(MSVC)
    set_source_files_properties(Calc.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()
