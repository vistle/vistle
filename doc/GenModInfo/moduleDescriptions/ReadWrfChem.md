
# ReadWrfChem
Read wrf-chem (weather research and forecasting model coupled to chemistry) data files

<svg width="2700" height="360" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="270" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="21.0" y="120" width="1.0" height="210" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="330" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="333.0" class="text" >grid<tspan> (grid_out)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="57.0" y="120" width="1.0" height="180" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="300" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="303.0" class="text" >scalar data<tspan> (data_out0)</tspan></text>
<rect x="78.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<rect x="93.0" y="120" width="1.0" height="150" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="270" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="273.0" class="text" >scalar data<tspan> (data_out1)</tspan></text>
<rect x="114.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out2</title></rect>
<rect x="129.0" y="120" width="1.0" height="120" rx="0" ry="0" style="fill:#000000;" />
<rect x="129.0" y="240" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="165.0" y="243.0" class="text" >scalar data<tspan> (data_out2)</tspan></text>
<rect x="150.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_U</title></rect>
<rect x="165.0" y="120" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="165.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="201.0" y="213.0" class="text" >scalar data<tspan> (data_out_U)</tspan></text>
<rect x="186.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_V</title></rect>
<rect x="201.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="201.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="237.0" y="183.0" class="text" >scalar data<tspan> (data_out_V)</tspan></text>
<rect x="222.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_W</title></rect>
<rect x="237.0" y="120" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="237.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="273.0" y="153.0" class="text" >scalar data<tspan> (data_out_W)</tspan></text>
<text x="6.0" y="85.5" class="moduleName" >ReadWrfChem</text></svg>

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
