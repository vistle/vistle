
# SplitDimensions
Separate grid into 1d, 2d and 3d components

## Input ports
|name|description|
|-|-|
|data_in||


<svg width="678.9999999999999" height="210" >
<rect x="0" y="0" width="678.9999999999999" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_3d</title></rect>
<rect x="98.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_2d</title></rect>
<rect x="182.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_1d</title></rect>
<rect x="266.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_0d</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">SplitDimensions</text></svg>

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
