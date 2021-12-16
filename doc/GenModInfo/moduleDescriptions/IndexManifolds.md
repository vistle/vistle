
# IndexManifolds
Extract lower dimensional surfaces, lines or points from structured grids

## Input ports
|name|description|
|-|-|
|data_in0||


<svg width="273.59999999999997" height="90" >
<rect x="0" y="0" width="273.59999999999997" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in0</title></rect>
<title>data_in0</title></rect><rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>surface_out0</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>line_out0</title></rect>
<rect x="78.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>point_out0</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">IndexManifolds</text></svg>

## Output ports
|name|description|
|-|-|
|surface_out0||
|line_out0||
|point_out0||


## Parameters
|name|description|type|
|-|-|-|
|coord|coordinates of point on surface/line|IntVector|
|direction|normal on surface and direction of line (X, Y, Z)|Integer|
