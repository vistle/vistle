
# ReadCovise
Read covise data



<svg width="204.0" height="90" >
<rect x="0" y="0" width="204.0" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>field0_out</title></rect>
<rect x="78.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>field1_out</title></rect>
<rect x="114.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>field2_out</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">ReadCovise</text></svg>

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
|filename|name of COVISE file|String|
|normals|name of COVISE file for normals|String|
|field0|name of COVISE file for field 2|String|
|field1|name of COVISE file for field 3|String|
|field2|name of COVISE file for field 4|String|
|distribute_time|distribute timesteps across MPI ranks|Integer|
