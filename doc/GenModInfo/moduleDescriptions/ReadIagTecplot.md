
# ReadIagTecplot
Read iag tecplot data (hexahedra only)

<svg width="2735.9999999999995" height="330" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="273.59999999999997" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="21.0" y="120" width="1.0" height="180" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="300" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="303.0" class="text" ><tspan> (grid_out)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>p</title></rect>
<rect x="57.0" y="120" width="1.0" height="150" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="270" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="273.0" class="text" ><tspan> (p)</tspan></text>
<rect x="78.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>rho</title></rect>
<rect x="93.0" y="120" width="1.0" height="120" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="240" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="243.0" class="text" ><tspan> (rho)</tspan></text>
<rect x="114.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>n</title></rect>
<rect x="129.0" y="120" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="129.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="165.0" y="213.0" class="text" ><tspan> (n)</tspan></text>
<rect x="150.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>u</title></rect>
<rect x="165.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="165.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="201.0" y="183.0" class="text" ><tspan> (u)</tspan></text>
<rect x="186.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>v</title></rect>
<rect x="201.0" y="120" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="201.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="237.0" y="153.0" class="text" ><tspan> (v)</tspan></text>
<text x="6.0" y="85.5" class="moduleName" >ReadIagTecplot</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|filename|name of Tecplot file|String|
