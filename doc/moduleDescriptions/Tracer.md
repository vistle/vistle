
# Tracer
Compute particle traces and streamlines

## Input ports
|name|description|
|-|-|
|data_in0||
|data_in1||


## Output ports
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


## Parameters
|name|description|type|
|-|-|-|
|taskType|task type (Streamlines, MovingPoints, Pathlines, Streaklines)|Integer|
|startpoint1|1st initial point|Vector|
|startpoint2|2nd initial point|Vector|
|direction|direction for plane|Vector|
|no_startp|number of startpoints|Integer|
|max_no_startp|maximum number of startpoints|Integer|
|steps_max|maximum number of integrations per particle|Integer|
|trace_len|maximum trace distance|Float|
|trace_time|maximum trace time|Float|
|tdirection|direction in which to trace (Both, Forward, Backward)|Integer|
|startStyle|initial particle position configuration (Line, Plane, Cylinder)|Integer|
|integration|integration method (Euler, RK32, ConstantVelocity)|Integer|
|min_speed|minimum particle speed|Float|
|dt_step|duration of a timestep (for moving points or when data does not specify realtime|Float|
|h_init|fixed step size for euler integration|Float|
|h_min|minimum step size for rk32 integration|Float|
|h_max|maximum step size for rk32 integration|Float|
|err_tol_abs|absolute error tolerance for rk32 integration|Float|
|err_tol_rel|relative error tolerance for rk32 integration|Float|
|cell_relative|whether step length control should take into account cell size|Integer|
|velocity_relative|whether step length control should take into account velocity|Integer|
|use_celltree|use celltree for accelerated cell location|Integer|
|num_active|number of particles to trace simultaneously on each node (0: no. of cores)|Integer|
|particle_placement|where a particle's data shall be collected (InitialRank, RankById)|Integer|
|cell_index_modulus|modulus for cell number output|Integer|
