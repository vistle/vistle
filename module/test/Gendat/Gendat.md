Gendat
======
generate scalar and vector test data on structured and unstructured grids

Output ports
------------
|name|description|
|-|-|
|grid_out|only grid|
|data_out0|scalar data|
|data_out1|vector data|

Parameters
----------
|name|type|description|
|-|-|-|
|first_step|Int|first timestep to read|
|last_step|Int|last timestep to read (-1: last)|
|step_increment|Int|number of steps to increment|
|first_rank|Int|rank for first partition of first timestep|
|check_convexity|Int|whether to check convexity of grid cells|
|geo_mode|Int|geometry generation mode|
|data_mode_scalar|Int|data generation mode|
|data_scale_scalar|Float|data scale factor (scalar)|
|data_mode_vec_x|Int|data generation mode|
|data_scale_vec_x|Float|data scale factor|
|data_mode_vec_y|Int|data generation mode|
|data_scale_vec_y|Float|data scale factor|
|data_mode_vec_z|Int|data generation mode|
|data_scale_vec_z|Float|data scale factor|
|size_x|Int|number of cells per block in x-direction|
|size_y|Int|number of cells per block in y-direction|
|size_z|Int|number of cells per block in z-direction|
|blocks_x|Int|number of blocks in x-direction|
|blocks_y|Int|number of blocks in y-direction|
|blocks_z|Int|number of blocks in z-direction|
|ghost_layers|Int|number of ghost layers on all sides of a grid|
|min|Vector|minimum coordinates|
|max|Vector|maximum coordinates|
|timesteps|Int|number of timesteps|
|element_data|Int|generate data per element/cell|
|anim_data|Int|data animation|
|delay|Float|wait after creating an object (s)|
