
# Gendat
generate scalar and vector test data on structured and unstructured grids
## Isosurface Example

This example uses the IsoSuface module to generate a surface with the data value 1.
To set this up create the pipeline as described in the following picture and change the isovalue parameter of the Surface module to 1.

<img src="../../../../module/test/Gendat/isosurfaceExample.png" alt="drawing" style="width:600px;"/>

Alternatively load vistle/examples/gendat-isisurface.vsl.
The result should look like this:

![]( ../../../module/test/Gendat/isosurfaceExampleResult.png )

<svg width="50.0em" height="7.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="0.8em" width="5.0em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<text x="0.2em" y="2.6500000000000004em" class="moduleName" >Gendat</text><rect x="0.2em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="0.7em" y="3.8em" width="0.03333333333333333em" height="3.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="0.7em" y="6.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="1.9em" y="6.8999999999999995em" class="text" >only grid<tspan> (grid_out)</tspan></text>
<rect x="1.4em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="1.9em" y="3.8em" width="0.03333333333333333em" height="2.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="1.9em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="5.8999999999999995em" class="text" >scalar data<tspan> (data_out0)</tspan></text>
<rect x="2.5999999999999996em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<rect x="3.0999999999999996em" y="3.8em" width="0.03333333333333333em" height="1.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="3.0999999999999996em" y="4.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="4.3em" y="4.8999999999999995em" class="text" >vector data<tspan> (data_out1)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Int|
|last_step|last timestep to read (-1: last)|Int|
|step_increment|number of steps to increment|Int|
|first_rank|rank for first partition of first timestep|Int|
|check_convexity|whether to check convexity of grid cells|Int|
|geo_mode|geometry generation mode (Triangle_Geometry, Quad_Geometry, Polygon_Geometry, Uniform_Grid, Rectilinear_Grid, Layer_Grid, Structured_Grid, Unstructured_Grid, Point_Geometry, Sphere_Geometry)|Int|
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
