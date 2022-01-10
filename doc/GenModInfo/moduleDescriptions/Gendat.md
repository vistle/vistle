
# Gendat
Generate scalar and vector test data on structured and unstructured grids
## Isosurface Example

This example uses the IsoSuface module to generate a surface with the data value 1.
To set this up create the pipeline as described in the following picture and change the isovalue parameter of the Surface module to 1.

<img src="../../../../module/test/Gendat/isosurfaceExample.png" alt="drawing" style="width:600px;"/>

Alternatively load vistle/examples/gendat-isisurface.vsl.
The result should look like this:

![](../../../module/test/Gendat/isosurfaceExampleResult.png)

<svg width="1500" height="240" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="150" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="21.0" y="120" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="213.0" class="text" >only grid<tspan> (grid_out)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="57.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="183.0" class="text" >scalar data<tspan> (data_out0)</tspan></text>
<rect x="78.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<rect x="93.0" y="120" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="153.0" class="text" >vector data<tspan> (data_out1)</tspan></text>
<text x="6.0" y="85.5" class="moduleName" >Gendat</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Int|
|last_step|last timestep to read (-1: last)|Int|
|step_increment|number of steps to increment|Int|
|first_rank|rank for first partition of first timestep|Int|
|check_convexity|whether to check convexity of grid cells|Int|
|geo_mode|geometry generation mode (Triangle_Geometry, Quad_Geometry, Polygon_Geometry, Uniform_Grid, Rectilinear_Grid, Structured_Grid, Unstructured_Grid, Point_Geometry, Sphere_Geometry)|Int|
|data_mode_scalar|data generation mode (One, Dist_Origin, Identity_X, Identity_Y, Identity_Z, Sine_X, Sine_Y, Sine_Z, Cosine_X, Cosine_Y, Cosine_Z, Random)|Int|
|data_scale_scalar|data scale factor (scalar)|Float|
|data_mode_vec_x|data generation mode (One, Dist_Origin, Identity_X, Identity_Y, Identity_Z, Sine_X, Sine_Y, Sine_Z, Cosine_X, Cosine_Y, Cosine_Z, Random)|Int|
|data_scale_vec_x|data scale factor|Float|
|data_mode_vec_y|data generation mode (One, Dist_Origin, Identity_X, Identity_Y, Identity_Z, Sine_X, Sine_Y, Sine_Z, Cosine_X, Cosine_Y, Cosine_Z, Random)|Int|
|data_scale_vec_y|data scale factor|Float|
|data_mode_vec_z|data generation mode (One, Dist_Origin, Identity_X, Identity_Y, Identity_Z, Sine_X, Sine_Y, Sine_Z, Cosine_X, Cosine_Y, Cosine_Z, Random)|Int|
|data_scale_vec_z|data scale factor|Float|
|size_x|number of cells per block in x-direction|Int|
|size_y|number of cells per block in y-direction|Int|
|size_z|number of cells per block in z-direction|Int|
|blocks_x|number of blocks in x-direction|Int|
|blocks_y|number of blocks in y-direction|Int|
|blocks_z|number of blocks in z-direction|Int|
|ghost_layers|number of ghost layers on all sides of a grid|Int|
|min|minimum coordinates|Vector|
|max|maximum coordinates|Vector|
|timesteps|number of timesteps|Int|
|element_data|generate data per element/cell|Int|
|anim_data|data animation (Constant, Add_Scale, Divide_Scale, Add_X, Add_Y, Add_Z)|Int|
|delay|wait after creating an object (s)|Float|
