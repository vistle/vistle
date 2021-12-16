
# ReadNek5000
Read .nek5000 files



<svg width="221.39999999999998" height="90" >
<rect x="0" y="0" width="221.39999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>velosity_out</title></rect>
<rect x="78.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>pressure_out</title></rect>
<rect x="114.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>temperature_out</title></rect>
<rect x="150.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>blockNumber_out</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">ReadNek5000</text></svg>

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
