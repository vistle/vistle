
# ReadNek5000
Read .nek5000 files

<svg width="2214.0" height="300" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="221.39999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="21.0" y="120" width="1.0" height="150" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="270" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="273.0" class="text" >grid<tspan> (grid_out)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>velosity_out</title></rect>
<rect x="57.0" y="120" width="1.0" height="120" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="240" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="243.0" class="text" >velocity<tspan> (velosity_out)</tspan></text>
<rect x="78.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>pressure_out</title></rect>
<rect x="93.0" y="120" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="213.0" class="text" >pressure data<tspan> (pressure_out)</tspan></text>
<rect x="114.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>temperature_out</title></rect>
<rect x="129.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="129.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="165.0" y="183.0" class="text" >temperature data<tspan> (temperature_out)</tspan></text>
<rect x="150.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>blockNumber_out</title></rect>
<rect x="165.0" y="120" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="165.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="201.0" y="153.0" class="text" >Nek internal block numbers<tspan> (blockNumber_out)</tspan></text>
<text x="6.0" y="85.5" class="moduleName" >ReadNek5000</text></svg>

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
