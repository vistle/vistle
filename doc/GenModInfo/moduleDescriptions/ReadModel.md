
# ReadModel
Read polygonal geometry models with assimp (stl, obj, 3ds, collada, dxf, ply, x3d, ...)



<svg width="186.6" height="90" >
<rect x="0" y="0" width="186.6" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">ReadModel</text></svg>

## Output ports
|name|description|
|-|-|
|grid_out||


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|filename|name of file (%1%: block, %2%: timestep)|String|
|indexed_geometry|create indexed geometry?|Integer|
|triangulate|only create triangles|Integer|
|first_block|number of first block|Integer|
|last_block|number of last block|Integer|
|ignore_errors|ignore files that are not found|Integer|
