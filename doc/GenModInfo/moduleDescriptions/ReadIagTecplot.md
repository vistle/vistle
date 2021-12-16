
# ReadIagTecplot
Read iag tecplot data (hexahedra only)



<svg width="638.3999999999999" height="210" >
<rect x="0" y="0" width="638.3999999999999" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="98.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>p</title></rect>
<rect x="182.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>rho</title></rect>
<rect x="266.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>n</title></rect>
<rect x="350.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>u</title></rect>
<rect x="434.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>v</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">ReadIagTecplot</text></svg>

## Output ports
|name|description|
|-|-|
|grid_out||
|p||
|rho||
|n||
|u||
|v||


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|filename|name of Tecplot file|String|
