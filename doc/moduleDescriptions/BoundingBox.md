
# BoundingBox
Compute bounding boxes

## Input ports
|name|description|
|-|-|
|grid_in|input data|


## Output ports
|name|description|
|-|-|
|grid_out|bounding box|


## Parameters
|name|description|type|
|-|-|-|
|per_block|create bounding box for each block|Integer|
|min|output parameter: minimum|Vector|
|max|output parameter: maximum|Vector|
|min_block|output parameter: block numbers containing minimum|IntVector|
|max_block|output parameter: block numbers containing maximum|IntVector|
|min_index|output parameter: indices of minimum|IntVector|
|max_index|output parameter: indices of maximum|IntVector|
