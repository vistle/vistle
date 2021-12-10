
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
|name|type|description|
|-|-|-|
|first_step|Int|first timestep to read|
|last_step|Int|last timestep to read (-1: last)|
|step_increment|Int|number of steps to increment|
|first_rank|Int|rank for first partition of first timestep|
|check_convexity|Int|whether to check convexity of grid cells|
|filename|String|name of Tecplot file|
