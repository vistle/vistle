
# ReadFoam
Read OpenFOAM data


## Output ports
|name|description|
|-|-|
|grid_out1||
|data_out0||
|data_out1||
|data_out2||
|data_2d_out0||
|data_2d_out1||
|data_2d_out2||



## Parameters
|name|type|description|
|-|-|-|
|first_step|Int|first timestep to read|
|last_step|Int|last timestep to read (-1: last)|
|step_increment|Int|number of steps to increment|
|first_rank|Int|rank for first partition of first timestep|
|check_convexity|Int|whether to check convexity of grid cells|
|casedir|String|OpenFOAM case directory|
|starttime|Float|start reading at the first step after this time|
|stoptime|Float|stop reading at the last step before this time|
|read_grid|Int|load the grid?|
|Data0|String|name of field|
|Data1|String|name of field|
|Data2|String|name of field|
|read_boundary|Int|load the boundary?|
|patches_as_variants|Int|create sub-objects with variant attribute for boundary patches|
|patches|String|select patches|
|Data2d0|String|name of field|
|Data2d1|String|name of field|
|Data2d2|String|name of field|
|build_ghostcells|Int|whether to build ghost cells|
|only_polyhedra|Int|create only polyhedral cells|
