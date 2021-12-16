
# Gendat
Generate scalar and vector test data on structured and unstructured grids



<svg width="313.59999999999997" height="210" >
<rect x="0" y="0" width="313.59999999999997" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="98.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="182.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">Gendat</text></svg>

## Output ports
|name|description|
|-|-|
|grid_out|only grid|
|data_out0|scalar data|
|data_out1|vector data|


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|geo_mode|geometry generation mode (Triangle_Geometry, Quad_Geometry, Polygon_Geometry, Uniform_Grid, Rectilinear_Grid, Structured_Grid, Unstructured_Grid, Point_Geometry, Sphere_Geometry)|Integer|
|data_mode_scalar|data generation mode (One, Dist_Origin, Identity_X, Identity_Y, Identity_Z, Sine_X, Sine_Y, Sine_Z, Cosine_X, Cosine_Y, Cosine_Z, Random)|Integer|
|data_scale_scalar|data scale factor (scalar)|Float|
|data_mode_vec_x|data generation mode (One, Dist_Origin, Identity_X, Identity_Y, Identity_Z, Sine_X, Sine_Y, Sine_Z, Cosine_X, Cosine_Y, Cosine_Z, Random)|Integer|
|data_scale_vec_x|data scale factor|Float|
|data_mode_vec_y|data generation mode (One, Dist_Origin, Identity_X, Identity_Y, Identity_Z, Sine_X, Sine_Y, Sine_Z, Cosine_X, Cosine_Y, Cosine_Z, Random)|Integer|
|data_scale_vec_y|data scale factor|Float|
|data_mode_vec_z|data generation mode (One, Dist_Origin, Identity_X, Identity_Y, Identity_Z, Sine_X, Sine_Y, Sine_Z, Cosine_X, Cosine_Y, Cosine_Z, Random)|Integer|
|data_scale_vec_z|data scale factor|Float|
|size_x|number of cells per block in x-direction|Integer|
|size_y|number of cells per block in y-direction|Integer|
|size_z|number of cells per block in z-direction|Integer|
|blocks_x|number of blocks in x-direction|Integer|
|blocks_y|number of blocks in y-direction|Integer|
|blocks_z|number of blocks in z-direction|Integer|
|ghost_layers|number of ghost layers on all sides of a grid|Integer|
|min|minimum coordinates|Vector|
|max|maximum coordinates|Vector|
|timesteps|number of timesteps|Integer|
|element_data|generate data per element/cell|Integer|
|anim_data|data animation (Constant, Add_Scale, Divide_Scale, Add_X, Add_Y, Add_Z)|Integer|
|delay|wait after creating an object (s)|Float|
## Isosurface Example

This example uses the IsoSuface module to generate a surface with the data value 1.
To set this up create the pipeline as described in the following picture and change the isovalue parameter of the Surface module to 1.

<img src="../../../module/test/Gendat/isosurfaceExample.png" alt="drawing" style="width:600px;"/>

Alternatively load vistle/examples/gendat-isisurface.vsl.
The result should look like this:

![](../../../module/test/Gendat/isosurfaceExampleResult.png)
