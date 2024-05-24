add_vtkm_module(VertToCellVtkm "convert vertex mapped data to cell mapped" ../CellToVertVtkm/CellToVertVtkm.cpp ../CellToVertVtkm/CellToVertVtkm.h)
target_compile_definitions(VertToCellVtkm PRIVATE VERTTOCELL)
