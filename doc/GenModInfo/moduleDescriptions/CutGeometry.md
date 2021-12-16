
# CutGeometry
Clip geometry at basic geometry like plane, cylinder or sphere

## Input ports
|name|description|
|-|-|
|grid_in||


<svg width="221.39999999999998" height="90" >
<rect x="0" y="0" width="221.39999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<title>grid_in</title></rect><rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">CutGeometry</text></svg>

## Output ports
|name|description|
|-|-|
|grid_out||


## Parameters
|name|description|type|
|-|-|-|
|point|point on plane|Vector|
|vertex|normal on plane|Vector|
|scalar|distance to origin of ordinates|Float|
|option|option (Plane, Sphere, CylinderX, CylinderY, CylinderZ)|Integer|
|direction|direction for variable Cylinder|Vector|
|flip|flip inside out|Integer|
