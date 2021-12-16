
# ReadCoviseDirectory
Read covise data in a directory



<svg width="841.3999999999999" height="210" >
<rect x="0" y="0" width="841.3999999999999" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="98.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>field0_out</title></rect>
<rect x="182.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>field1_out</title></rect>
<rect x="266.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>field2_out</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">ReadCoviseDirectory</text></svg>

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