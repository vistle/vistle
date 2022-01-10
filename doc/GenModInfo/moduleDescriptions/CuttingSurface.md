
# CuttingSurface
Cut through grids with basic geometry like plane, cylinder or sphere

<svg width="2735.9999999999995" height="210" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="60" width="273.59999999999997" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="21.0" y="30" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="30" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="33.0" class="text" ><tspan> (data_in)</tspan></text>
<rect x="6.0" y="120" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="21.0" y="150" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="183.0" class="text" ><tspan> (data_out)</tspan></text>
<text x="6.0" y="115.5" class="moduleName" >CuttingSurface</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|point|point on plane|Vector|
|vertex|normal on plane|Vector|
|scalar|distance to origin of ordinates|Float|
|option|option (Plane, Sphere, CylinderX, CylinderY, CylinderZ)|Int|
|direction|direction for variable Cylinder|Vector|
|processortype|processortype (Host, Device)|Int|
|compute_normals|compute normals (structured grids only)|Int|
