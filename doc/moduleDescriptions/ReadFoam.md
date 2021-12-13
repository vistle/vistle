
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
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|casedir|OpenFOAM case directory|String|
|starttime|start reading at the first step after this time|Float|
|stoptime|stop reading at the last step before this time|Float|
|read_grid|load the grid?|Integer|
|Data0|name of field ((NONE))|String|
|Data1|name of field ((NONE))|String|
|Data2|name of field ((NONE))|String|
|read_boundary|load the boundary?|Integer|
|patches_as_variants|create sub-objects with variant attribute for boundary patches|Integer|
|patches|select patches|String|
|Data2d0|name of field ((NONE))|String|
|Data2d1|name of field ((NONE))|String|
|Data2d2|name of field ((NONE))|String|
|build_ghostcells|whether to build ghost cells|Integer|
|only_polyhedra|create only polyhedral cells|Integer|
