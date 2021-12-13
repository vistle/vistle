
# ReadNek5000
Read .nek5000 files



## Output ports
|name|description|
|-|-|
|grid_out|grid|
|velosity_out|velocity|
|pressure_out|pressure data|
|temperature_out|temperature data|
|blockNumber_out|Nek internal block numbers|


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|filename|Geometry file path|String|
|OnlyGeometry|Reading only Geometry? yes|no|Integer|
|numGhostLayers|number of ghost layers around eeach partition, a layer consists of whole blocks|Integer|
|number of blocks|number of blocks to read from file, <= 0 to read all|Integer|
|numberOfPartitions|number of parallel partitions to use for reading, 0 = one partition for each rank|Integer|
