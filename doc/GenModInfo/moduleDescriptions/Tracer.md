
# Tracer
Compute particle traces and streamlines

## Input ports
|name|description|
|-|-|
|data_in0||
|data_in1||


<svg width="330" height="90" >
<rect x="0" y="0" width="330" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in0</title></rect>
<title>data_in0</title></rect><rect x="42.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in1</title></rect>
<title>data_in1</title></rect><rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<rect x="78.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>particle_id</title></rect>
<rect x="114.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>step</title></rect>
<rect x="150.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>time</title></rect>
<rect x="186.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>stepwidth</title></rect>
<rect x="222.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>distance</title></rect>
<rect x="258.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>stop_reason</title></rect>
<rect x="294.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>cell_index</title></rect>
<rect x="330.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>block_index</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">Tracer</text></svg>

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
