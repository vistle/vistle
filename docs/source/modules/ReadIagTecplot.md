
# ReadIagTecplot
read IAG Tecplot data (hexahedra only)

<svg width="91.19999999999999em" height="10.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="0.8em" width="9.12em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<text x="0.2em" y="2.6500000000000004em" class="moduleName" >ReadIagTecplot</text><rect x="0.2em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="0.7em" y="3.8em" width="0.03333333333333333em" height="6.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="9.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="9.9em" class="text" >grid or geometry<tspan> (grid_out)</tspan></text>
<rect x="1.4em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>p</title></rect>
<rect x="1.9em" y="3.8em" width="0.03333333333333333em" height="5.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="8.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="8.9em" class="text" >pressure<tspan> (p)</tspan></text>
<rect x="2.5999999999999996em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>rho</title></rect>
<rect x="3.0999999999999996em" y="3.8em" width="0.03333333333333333em" height="4.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="3.0999999999999996em" y="7.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="4.3em" y="7.8999999999999995em" class="text" >rho<tspan> (rho)</tspan></text>
<rect x="3.8em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>n</title></rect>
<rect x="4.3em" y="3.8em" width="0.03333333333333333em" height="3.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="4.3em" y="6.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="5.5em" y="6.8999999999999995em" class="text" >n<tspan> (n)</tspan></text>
<rect x="5.0em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>u</title></rect>
<rect x="5.5em" y="3.8em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="5.5em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="6.7em" y="5.8999999999999995em" class="text" >u<tspan> (u)</tspan></text>
<rect x="6.2em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>v</title></rect>
<rect x="6.7em" y="3.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="6.7em" y="4.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="7.9em" y="4.8999999999999995em" class="text" >v<tspan> (v)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Int|
|last_step|last timestep to read (-1: last)|Int|
|step_increment|number of steps to increment|Int|
|first_rank|rank for first partition of first timestep|Int|
|check_convexity|whether to check convexity of grid cells|Int|
|filename|name of Tecplot file|String|
