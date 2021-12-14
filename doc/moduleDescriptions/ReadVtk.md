
# ReadVtk
Read VTK data



## Output ports
|name|description|
|-|-|
|grid_out||
|point_data0|vertex data|
|point_data1|vertex data|
|point_data2|vertex data|
|cell_data0|cell data|
|cell_data1|cell data|
|cell_data2|cell data|


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|filename|name of VTK file|String|
|read_pieces|create block for every piece in an unstructured grid|Integer|
|create_ghost_cells|create ghost cells for multi-piece unstructured grids|Integer|
|point_field_0|point data field)|String|
|point_field_1|point data field)|String|
|point_field_2|point data field)|String|
|cell_field_0|cell data field)|String|
|cell_field_1|cell data field)|String|
|cell_field_2|cell data field)|String|
