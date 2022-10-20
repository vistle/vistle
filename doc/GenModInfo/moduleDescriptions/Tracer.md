
# Tracer
compute particle traces and streamlines

<svg width="120.0em" height="16.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="2.8em" width="12.0em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>data_in0</title></rect>
<rect x="0.7em" y="0.7999999999999998em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="0.7999999999999998em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="0.8999999999999998em" class="text" >vector field<tspan> (data_in0)</tspan></text>
<rect x="1.4em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>data_in1</title></rect>
<rect x="1.9em" y="1.7999999999999998em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="1.7999999999999998em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="1.9em" class="text" >additional field on same geometry<tspan> (data_in1)</tspan></text>
<text x="0.2em" y="4.65em" class="moduleName" >Tracer</text><rect x="0.2em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="0.7em" y="5.8em" width="0.03333333333333333em" height="10.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="15.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="15.9em" class="text" >stream lines or points with mapped vector<tspan> (data_out0)</tspan></text>
<rect x="1.4em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<rect x="1.9em" y="5.8em" width="0.03333333333333333em" height="9.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="14.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="14.9em" class="text" >stream lines or points with mapped field<tspan> (data_out1)</tspan></text>
<rect x="2.5999999999999996em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>particle_id</title></rect>
<rect x="3.0999999999999996em" y="5.8em" width="0.03333333333333333em" height="8.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="3.0999999999999996em" y="13.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="4.3em" y="13.9em" class="text" >stream lines or points with mapped particle id<tspan> (particle_id)</tspan></text>
<rect x="3.8em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>step</title></rect>
<rect x="4.3em" y="5.8em" width="0.03333333333333333em" height="7.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="4.3em" y="12.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="5.5em" y="12.9em" class="text" >stream lines or points with step number<tspan> (step)</tspan></text>
<rect x="5.0em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>time</title></rect>
<rect x="5.5em" y="5.8em" width="0.03333333333333333em" height="6.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="5.5em" y="11.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="6.7em" y="11.9em" class="text" >stream lines or points with time<tspan> (time)</tspan></text>
<rect x="6.2em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>stepwidth</title></rect>
<rect x="6.7em" y="5.8em" width="0.03333333333333333em" height="5.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="6.7em" y="10.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="7.9em" y="10.9em" class="text" >stream lines or points with stepwidth<tspan> (stepwidth)</tspan></text>
<rect x="7.4em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>distance</title></rect>
<rect x="7.9em" y="5.8em" width="0.03333333333333333em" height="4.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="7.9em" y="9.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="9.1em" y="9.9em" class="text" >stream lines or points with accumulated distance<tspan> (distance)</tspan></text>
<rect x="8.6em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>stop_reason</title></rect>
<rect x="9.1em" y="5.8em" width="0.03333333333333333em" height="3.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="9.1em" y="8.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="10.299999999999999em" y="8.9em" class="text" >stream lines or points with reason for finishing trace<tspan> (stop_reason)</tspan></text>
<rect x="9.799999999999999em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>cell_index</title></rect>
<rect x="10.299999999999999em" y="5.8em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="10.299999999999999em" y="7.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="11.499999999999998em" y="7.8999999999999995em" class="text" >stream lines or points with cell index<tspan> (cell_index)</tspan></text>
<rect x="10.999999999999998em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>block_index</title></rect>
<rect x="11.499999999999998em" y="5.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="11.499999999999998em" y="6.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="12.699999999999998em" y="6.8999999999999995em" class="text" >stream lines or points with block index<tspan> (block_index)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|taskType|task type (Streamlines, MovingPoints, Pathlines, Streaklines)|Int|
|startpoint1|1st initial point|Vector|
|startpoint2|2nd initial point|Vector|
|direction|direction for plane|Vector|
|no_startp|number of startpoints|Int|
|max_no_startp|maximum number of startpoints|Int|
|steps_max|maximum number of integrations per particle|Int|
|trace_len|maximum trace distance|Float|
|trace_time|maximum trace time|Float|
|tdirection|direction in which to trace (Both, Forward, Backward)|Int|
|startStyle|initial particle position configuration (Line, Plane, Cylinder)|Int|
|integration|integration method (Euler, RK32, ConstantVelocity)|Int|
|min_speed|minimum particle speed|Float|
|dt_step|duration of a timestep (for moving points or when data does not specify realtime|Float|
|h_init|fixed step size for euler integration|Float|
|h_min|minimum step size for rk32 integration|Float|
|h_max|maximum step size for rk32 integration|Float|
|err_tol_abs|absolute error tolerance for rk32 integration|Float|
|err_tol_rel|relative error tolerance for rk32 integration|Float|
|cell_relative|whether step length control should take into account cell size|Int|
|velocity_relative|whether step length control should take into account velocity|Int|
|use_celltree|use celltree for accelerated cell location|Int|
|num_active|number of particles to trace simultaneously on each node (0: no. of cores)|Int|
|particle_placement|where a particle's data shall be collected (InitialRank, RankById)|Int|
|cell_index_modulus|modulus for cell number output|Int|
|simplification_error|tolerable relative error for result simplification|Float|
