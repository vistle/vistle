
# ReadIagTecplot
Read IAG Tecplot data (hexahedra only)



## Output ports
|name|description|
|-|-|
|grid_out||
|p||
|rho||
|n||
|u||
|v||


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|filename|name of Tecplot file|String|
