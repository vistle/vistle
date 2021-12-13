
# ReadCoviseDirectory
Read COVISE data in a directory



## Output ports
|name|description|
|-|-|
|grid_out||
|field0_out||
|field1_out||
|field2_out||


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|directory|directory to scan for .covise files|String|
|grid|filename for grid ((none))|String|
|normals|name of COVISE file for normals ((none))|String|
|field0|name of COVISE file for field 2 ((none))|String|
|field1|name of COVISE file for field 3 ((none))|String|
|field2|name of COVISE file for field 4 ((none))|String|
|distribute_time|distribute timesteps across MPI ranks|Integer|
