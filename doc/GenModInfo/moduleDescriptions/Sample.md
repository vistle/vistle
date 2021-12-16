
# Sample
Sample data on points, unstructured and uniform grids to a uniform grid

## Input ports
|name|description|
|-|-|
|data_in|object with data to be sampled|
|ref_in|target grid|


<svg width="134.39999999999998" height="90" >
<rect x="0" y="0" width="134.39999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="42.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>ref_in</title></rect>
<title>ref_in</title></rect><rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">Sample</text></svg>

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
