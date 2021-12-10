
# ReadItlrFs3d
Read ITLR FS3D (Free Surface 3D) binary data


## Output ports
|name|description|
|-|-|
|data0|data output|
|data1|data output|
|data2|data output|



## Parameters
|name|type|description|
|-|-|-|
|first_step|Int|first timestep to read|
|last_step|Int|last timestep to read (-1: last)|
|step_increment|Int|number of steps to increment|
|first_rank|Int|rank for first partition of first timestep|
|check_convexity|Int|whether to check convexity of grid cells|
|increment_filename|Int|replace digits in filename with timestep|
|grid_filename|String|.bin file for grid|
|filename0|String|.lst or .bin file for data|
|filename1|String|.lst or .bin file for data|
|filename2|String|.lst or .bin file for data|
|num_partitions|Int|number of partitions (-1: MPI ranks)|
|distribute_time|Int|distribute timesteps across MPI ranks|
