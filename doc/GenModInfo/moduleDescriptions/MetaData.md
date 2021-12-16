
# MetaData
Transform meta data to mappable data

## Input ports
|name|description|
|-|-|
|grid_in||


<svg width="169.2" height="90" >
<rect x="0" y="0" width="169.2" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<title>grid_in</title></rect><rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">MetaData</text></svg>

## Output ports
|name|description|
|-|-|
|data_out||


## Parameters
|name|description|type|
|-|-|-|
|attribute|attribute to map to vertices (MpiRank, BlockNumber, TimestepNumber, VertexIndex, ElementIndex, ElementType)|Integer|
|range|range to which data shall be clamped|IntVector|
|modulus|wrap around output value|Integer|
