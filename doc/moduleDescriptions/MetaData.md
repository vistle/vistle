
# MetaData
Transform meta data to mappable data

## Input ports
|name|description|
|-|-|
|grid_in||


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
