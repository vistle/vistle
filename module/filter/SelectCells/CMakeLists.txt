add_module(SelectCells "filter elements according to their index" ../Threshold/Threshold.cpp ../Threshold/Threshold.h)
target_compile_definitions(SelectCells PRIVATE CELLSELECT)
