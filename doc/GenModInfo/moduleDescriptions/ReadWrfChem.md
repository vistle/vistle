
# ReadWrfChem
Read wrf-chem (weather research and forecasting model coupled to chemistry) data files



<svg width="240" height="90" >
<rect x="0" y="0" width="240" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="78.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<rect x="114.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out2</title></rect>
<rect x="150.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_U</title></rect>
<rect x="186.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_V</title></rect>
<rect x="222.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_W</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">ReadWrfChem</text></svg>

## Output ports
|name|description|
|-|-|
|grid_out|grid|
|data_out0|scalar data|
|data_out1|scalar data|
|data_out2|scalar data|
|data_out_U|scalar data|
|data_out_V|scalar data|
|data_out_W|scalar data|


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|file_dir|NC files directory|String|
|num_partitions_lat|number of partitions in lateral|Integer|
|num_partitions_ver|number of partitions in vertical|Integer|
|var_dim|Dimension of variables (2D, 3D, other)|String|
|true_height|Use real ground topology ((NONE))|String|
|GridX|grid Sout-North axis ((NONE))|String|
|GridY|grid East_West axis ((NONE))|String|
|pert_gp|perturbation geopotential ((NONE))|String|
|base_gp|base-state geopotential ((NONE))|String|
|GridZ|grid Bottom-Top axis ((NONE))|String|
|Variable0|Variable0 ((NONE))|String|
|Variable1|Variable1 ((NONE))|String|
|Variable2|Variable2 ((NONE))|String|
|U|U ((NONE))|String|
|V|V ((NONE))|String|
|W|W ((NONE))|String|
|distribute_time|distribute timesteps across MPI ranks|Integer|
