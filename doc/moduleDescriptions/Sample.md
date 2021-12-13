
# Sample
Sample data on points, unstructured and uniform grids to a uniform grid

## Input ports
|name|description|
|-|-|
|data_in|object with data to be sampled|
|ref_in|target grid|


## Output ports
|name|description|
|-|-|
|data_out||


## Parameters
|name|description|type|
|-|-|-|
|mode|interpolation mode (First, Mean, Nearest, Linear)|Integer|
|create_celltree|create celltree|Integer|
|value_outside|value to be used if target is outside source domain (NaN, Zero, userDefined)|Integer|
|user_defined_value|user defined value if target outside source domain|Float|
|mulit_hits|handling if multiple interpolatied values found for one target point  (Any, Average)|Integer|
