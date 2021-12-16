
# ReadItlrFs3d
Read itlr fs3d (free surface 3d) binary data



<svg width="238.79999999999998" height="90" >
<rect x="0" y="0" width="238.79999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data0</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data1</title></rect>
<rect x="78.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data2</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">ReadItlrFs3d</text></svg>

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
