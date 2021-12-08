CuttingSurface
==============
cut through grids with basic geometry like plane, cylinder or sphere

Input ports
-----------
|name|description|
|-|-|
|data_in||

Output ports
------------
|name|description|
|-|-|
|data_out||

Parameters
----------
|name|type|description|
|-|-|-|
|point|Vector|point on plane|
|vertex|Vector|normal on plane|
|scalar|Float|distance to origin of ordinates|
|option|Int|option|
|direction|Vector|direction for variable Cylinder|
|processortype|Int|processortype|
|compute_normals|Int|compute normals (structured grids only)|
