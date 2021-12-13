
# CutGeometry
Clip geometry at basic geometry like plane, cylinder or sphere

## Input ports
|name|description|
|-|-|
|grid_in||


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
