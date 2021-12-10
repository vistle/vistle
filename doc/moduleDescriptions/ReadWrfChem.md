
# ReadWrfChem
Read WRF-Chem (Weather Research and Forecasting model coupled to Chemistry) data files


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
|name|type|description|
|-|-|-|
|first_step|Int|first timestep to read|
|last_step|Int|last timestep to read (-1: last)|
|step_increment|Int|number of steps to increment|
|first_rank|Int|rank for first partition of first timestep|
|check_convexity|Int|whether to check convexity of grid cells|
|file_dir|String|NC files directory|
|num_partitions_lat|Int|number of partitions in lateral|
|num_partitions_ver|Int|number of partitions in vertical|
|var_dim|String|Dimension of variables|
|true_height|String|Use real ground topology|
|GridX|String|grid Sout-North axis|
|GridY|String|grid East_West axis|
|pert_gp|String|perturbation geopotential|
|base_gp|String|base-state geopotential|
|GridZ|String|grid Bottom-Top axis|
|Variable0|String|Variable0|
|Variable1|String|Variable1|
|Variable2|String|Variable2|
|U|String|U|
|V|String|V|
|W|String|W|
|distribute_time|Int|distribute timesteps across MPI ranks|
