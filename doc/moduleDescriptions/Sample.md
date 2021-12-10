
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
|name|type|description|
|-|-|-|
|mode|Int|interpolation mode|
|create_celltree|Int|create celltree|
|value_outside|Int|value to be used if target is outside source domain|
|user_defined_value|Float|user defined value if target outside source domain|
|mulit_hits|Int|handling if multiple interpolatied values found for one target point |
