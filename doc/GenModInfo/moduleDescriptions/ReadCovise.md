
# ReadCovise
Read covise data

<svg width="2040.0" height="270" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="204.0" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="21.0" y="120" width="1.0" height="120" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="240" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="243.0" class="text" ><tspan> (grid_out)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>field0_out</title></rect>
<rect x="57.0" y="120" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="213.0" class="text" ><tspan> (field0_out)</tspan></text>
<rect x="78.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>field1_out</title></rect>
<rect x="93.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="183.0" class="text" ><tspan> (field1_out)</tspan></text>
<rect x="114.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>field2_out</title></rect>
<rect x="129.0" y="120" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="129.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="165.0" y="153.0" class="text" ><tspan> (field2_out)</tspan></text>
<text x="6.0" y="85.5" class="moduleName" >ReadCovise</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Int|
|last_step|last timestep to read (-1: last)|Int|
|step_increment|number of steps to increment|Int|
|first_rank|rank for first partition of first timestep|Int|
|check_convexity|whether to check convexity of grid cells|Int|
|filename|name of COVISE file|String|
|normals|name of COVISE file for normals|String|
|field0|name of COVISE file for field 2|String|
|field1|name of COVISE file for field 3|String|
|field2|name of COVISE file for field 4|String|
|distribute_time|distribute timesteps across MPI ranks|Int|
