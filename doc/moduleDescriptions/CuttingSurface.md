
# CuttingSurface
Cut through grids with basic geometry like plane, cylinder or sphere

## Input ports
|name|description|
|-|-|
|data_in||


## Output ports
|name|description|
|-|-|
|data_out||


## Parameters
|name|description|type|
|-|-|-|
|point|point on plane|Vector|
|vertex|normal on plane|Vector|
|scalar|distance to origin of ordinates|Float|
|option|option (Plane, Sphere, CylinderX, CylinderY, CylinderZ)|Integer|
|direction|direction for variable Cylinder|Vector|
|processortype|processortype (Host, Device)|Integer|
|compute_normals|compute normals (structured grids only)|Integer|
