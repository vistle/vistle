
# MetaData
Transform meta data to mappable data

## Input ports
|name|description|
|-|-|
|grid_in||


<svg width="394.79999999999995" height="210" >
<rect x="0" y="0" width="394.79999999999995" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<title>grid_in</title></rect><rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">MetaData</text></svg>

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
