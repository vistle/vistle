
# ReadModel
Read polygonal geometry models with Assimp (STL, OBJ, 3DS, Collada, DXF, PLY, X3D, ...)



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
