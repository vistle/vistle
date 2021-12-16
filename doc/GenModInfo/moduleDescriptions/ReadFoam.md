
# ReadFoam
Read openfoam data



<svg width="240" height="90" >
<rect x="0" y="0" width="240" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out1</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="78.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<rect x="114.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out2</title></rect>
<rect x="150.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_2d_out0</title></rect>
<rect x="186.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_2d_out1</title></rect>
<rect x="222.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_2d_out2</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">ReadFoam</text></svg>

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
