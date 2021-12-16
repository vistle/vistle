
# ReadIagTecplot
Read iag tecplot data (hexahedra only)



<svg width="273.59999999999997" height="90" >
<rect x="0" y="0" width="273.59999999999997" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>p</title></rect>
<rect x="78.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>rho</title></rect>
<rect x="114.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>n</title></rect>
<rect x="150.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>u</title></rect>
<rect x="186.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>v</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">ReadIagTecplot</text></svg>

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
