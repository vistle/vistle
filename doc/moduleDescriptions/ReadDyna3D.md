
# ReadDyna3D
Read LS-Dyna3D ptf files


## Output ports
|name|description|
|-|-|
|grid_out|grid|
|data1_out|vector data|
|data2_out|scalar data|



## Parameters
|name|type|description|
|-|-|-|
|first_step|Int|first timestep to read|
|last_step|Int|last timestep to read (-1: last)|
|step_increment|Int|number of steps to increment|
|first_rank|Int|rank for first partition of first timestep|
|check_convexity|Int|whether to check convexity of grid cells|
|filename|String|Geometry file path|
|nodalDataType|Int|Nodal results data to be read|
|elementDataType|Int|Element results data to be read|
|component|Int|stress tensor component|
|Selection|String|Number selection for parts|
|format|Int|Format of LS-DYNA3D ptf-File|
|byteswap|Int|Perform Byteswapping|
|OnlyGeometry|Int|Reading only Geometry? yes|no|
