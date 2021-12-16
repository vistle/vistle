
# IndexManifolds
Extract lower dimensional surfaces, lines or points from structured grids

## Input ports
|name|description|
|-|-|
|data_in0||


<svg width="638.3999999999999" height="210" >
<rect x="0" y="0" width="638.3999999999999" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in0</title></rect>
<title>data_in0</title></rect><rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>surface_out0</title></rect>
<rect x="98.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>line_out0</title></rect>
<rect x="182.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>point_out0</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">IndexManifolds</text></svg>

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
