Tracer
======
compute particle traces and streamlines

Input ports
-----------
|name|description|
|-|-|
|data_in0||
|data_in1||

Output ports
------------
|name|description|
|-|-|
|data_out0||
|data_out1||
|particle_id||
|step||
|time||
|stepwidth||
|distance||
|stop_reason||
|cell_index||
|block_index||

Parameters
----------
|name|type|description|
|-|-|-|
|taskType|Int|task type|
|startpoint1|Vector|1st initial point|
|startpoint2|Vector|2nd initial point|
|direction|Vector|direction for plane|
|no_startp|Int|number of startpoints|
|max_no_startp|Int|maximum number of startpoints|
|steps_max|Int|maximum number of integrations per particle|
|trace_len|Float|maximum trace distance|
|trace_time|Float|maximum trace time|
|tdirection|Int|direction in which to trace|
|startStyle|Int|initial particle position configuration|
|integration|Int|integration method|
|min_speed|Float|minimum particle speed|
|dt_step|Float|duration of a timestep (for moving points or when data does not specify realtime|
|h_init|Float|fixed step size for euler integration|
|h_min|Float|minimum step size for rk32 integration|
|h_max|Float|maximum step size for rk32 integration|
|err_tol_abs|Float|absolute error tolerance for rk32 integration|
|err_tol_rel|Float|relative error tolerance for rk32 integration|
|cell_relative|Int|whether step length control should take into account cell size|
|velocity_relative|Int|whether step length control should take into account velocity|
|use_celltree|Int|use celltree for accelerated cell location|
|num_active|Int|number of particles to trace simultaneously on each node (0: no. of cores)|
|particle_placement|Int|where a particle's data shall be collected|
|cell_index_modulus|Int|modulus for cell number output|
