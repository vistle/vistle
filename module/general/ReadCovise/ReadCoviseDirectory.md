[headline]:<>
ReadCoviseDirectory - read COVISE data in a directory
=====================================================
[headline]:<>
[inputPorts]:<>
[inputPorts]:<>
[outputPorts]:<>
Output ports
------------
|name|description|
|-|-|
|grid_out||
|field0_out||
|field1_out||
|field2_out||


[outputPorts]:<>
[parameters]:<>
Parameters
----------
|name|type|description|
|-|-|-|
|first_step|Int|first timestep to read|
|last_step|Int|last timestep to read (-1: last)|
|step_increment|Int|number of steps to increment|
|first_rank|Int|rank for first partition of first timestep|
|check_convexity|Int|whether to check convexity of grid cells|
|directory|String|directory to scan for .covise files|
|grid|String|filename for grid|
|normals|String|name of COVISE file for normals|
|field0|String|name of COVISE file for field 2|
|field1|String|name of COVISE file for field 3|
|field2|String|name of COVISE file for field 4|
|distribute_time|Int|distribute timesteps across MPI ranks|

[parameters]:<>
<>
