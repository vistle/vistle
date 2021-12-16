
# SplitDimensions
Separate grid into 1d, 2d and 3d components

## Input ports
|name|description|
|-|-|
|data_in||


<svg width="291.0" height="90" >
<rect x="0" y="0" width="291.0" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_3d</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_2d</title></rect>
<rect x="78.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_1d</title></rect>
<rect x="114.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_0d</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">SplitDimensions</text></svg>

## Output ports
|name|description|
|-|-|
|data_out_3d||
|data_out_2d||
|data_out_1d||
|data_out_0d||


## Parameters
|name|description|type|
|-|-|-|
|reuse_coordinates|do not renumber vertices and reuse coordinate and data arrays|Integer|
