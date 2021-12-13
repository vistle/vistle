
# ReadItlrFs3d
Read ITLR FS3D (Free Surface 3D) binary data



## Output ports
|name|description|
|-|-|
|data0|data output|
|data1|data output|
|data2|data output|


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|increment_filename|replace digits in filename with timestep|Integer|
|grid_filename|.bin file for grid|String|
|filename0|.lst or .bin file for data|String|
|filename1|.lst or .bin file for data|String|
|filename2|.lst or .bin file for data|String|
|num_partitions|number of partitions (-1: MPI ranks)|Integer|
|distribute_time|distribute timesteps across MPI ranks|Integer|
