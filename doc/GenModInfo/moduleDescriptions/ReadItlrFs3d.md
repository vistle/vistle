
# ReadItlrFs3d
Read itlr fs3d (free surface 3d) binary data

<svg width="2388.0" height="240" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="238.79999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data0</title></rect>
<rect x="21.0" y="120" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="213.0" class="text" >data output<tspan> (data0)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data1</title></rect>
<rect x="57.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="183.0" class="text" >data output<tspan> (data1)</tspan></text>
<rect x="78.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data2</title></rect>
<rect x="93.0" y="120" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="153.0" class="text" >data output<tspan> (data2)</tspan></text>
<text x="6.0" y="85.5" class="moduleName" >ReadItlrFs3d</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Int|
|last_step|last timestep to read (-1: last)|Int|
|step_increment|number of steps to increment|Int|
|first_rank|rank for first partition of first timestep|Int|
|check_convexity|whether to check convexity of grid cells|Int|
|increment_filename|replace digits in filename with timestep|Int|
|grid_filename|.bin file for grid|String|
|filename0|.lst or .bin file for data|String|
|filename1|.lst or .bin file for data|String|
|filename2|.lst or .bin file for data|String|
|num_partitions|number of partitions (-1: MPI ranks)|Int|
|distribute_time|distribute timesteps across MPI ranks|Int|
