
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
|name|type|description|
|-|-|-|
|per_block|Int|create bounding box for each block|
|min|Vector|output parameter: minimum|
|max|Vector|output parameter: maximum|
|min_block|IntVector|output parameter: block numbers containing minimum|
|max_block|IntVector|output parameter: block numbers containing maximum|
|min_index|IntVector|output parameter: indices of minimum|
|max_index|IntVector|output parameter: indices of maximum|
