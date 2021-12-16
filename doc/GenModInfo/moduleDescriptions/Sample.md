
# Sample
Sample data on points, unstructured and uniform grids to a uniform grid

## Input ports
|name|description|
|-|-|
|data_in|object with data to be sampled|
|ref_in|target grid|


<svg width="313.59999999999997" height="210" >
<rect x="0" y="0" width="313.59999999999997" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="98.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>ref_in</title></rect>
<title>ref_in</title></rect><rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">Sample</text></svg>

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
