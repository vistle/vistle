ReadNek5000
===========
read .nek5000 files

Output ports
------------
|name|description|
|-|-|
|grid_out|grid|
|velosity_out|velocity|
|pressure_out|pressure data|
|temperature_out|temperature data|
|blockNumber_out|Nek internal block numbers|

Parameters
----------
|name|type|description|
|-|-|-|
|first_step|Int|first timestep to read|
|last_step|Int|last timestep to read (-1: last)|
|step_increment|Int|number of steps to increment|
|first_rank|Int|rank for first partition of first timestep|
|check_convexity|Int|whether to check convexity of grid cells|
|filename|String|Geometry file path|
|OnlyGeometry|Int|Reading only Geometry? yes|no|
|numGhostLayers|Int|number of ghost layers around eeach partition, a layer consists of whole blocks|
|number of blocks|Int|number of blocks to read from file, <= 0 to read all|
|numberOfPartitions|Int|number of parallel partitions to use for reading, 0 = one partition for each rank|
