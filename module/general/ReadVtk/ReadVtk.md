[headline]:<>
ReadVtk - read VTK data
=======================
[headline]:<>
[inputPorts]:<>
[inputPorts]:<>
[outputPorts]:<>
Output ports
------------
|name|description|
|-|-|
|grid_out||
|point_data0|vertex data|
|point_data1|vertex data|
|point_data2|vertex data|
|cell_data0|cell data|
|cell_data1|cell data|
|cell_data2|cell data|


[outputPorts]:<>
[parameters]:<>
Parameters
----------
|name|type|description|
|-|-|-|
|first_step|Int|first timestep to read|
|last_step|Int|last timestep to read (-1: last)|
|step_increment|Int|number of steps to increment|
|first_rank|Int|rank for first partition of first timestep|
|check_convexity|Int|whether to check convexity of grid cells|
|filename|String|name of VTK file|
|read_pieces|Int|create block for every piece in an unstructured grid|
|create_ghost_cells|Int|create ghost cells for multi-piece unstructured grids|
|point_field_0|String|point data field|
|point_field_1|String|point data field|
|point_field_2|String|point data field|
|cell_field_0|String|cell data field|
|cell_field_1|String|cell data field|
|cell_field_2|String|cell data field|

[parameters]:<>
s]:<>
