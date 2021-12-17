
# ReadDyna3D
Read ls-dyna3d ptf files

<svg width="2040.0" height="240" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="204.0" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="21.0" y="120" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="213.0" class="text" >grid<tspan> (grid_out)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data1_out</title></rect>
<rect x="57.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="183.0" class="text" >vector data<tspan> (data1_out)</tspan></text>
<rect x="78.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data2_out</title></rect>
<rect x="93.0" y="120" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="153.0" class="text" >scalar data<tspan> (data2_out)</tspan></text>
<text x="6.0" y="85.5" class="moduleName" >ReadDyna3D</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|filename|Geometry file path|String|
|nodalDataType|Nodal results data to be read (No_Node_Data, Displacements, Velocities, Accelerations)|Integer|
|elementDataType|Element results data to be read (No_Element_Data, Stress_Tensor, Plastic_Strain, Thickness)|Integer|
|component|stress tensor component (Sx, Sy, Sz, Txy, Tyz, Txz, Pressure, Von_Mises)|Integer|
|Selection|Number selection for parts|String|
|format|Format of LS-DYNA3D ptf-File (CADFEM, Original, Guess)|Integer|
|byteswap|Perform Byteswapping (Off, On, Auto)|Integer|
|OnlyGeometry|Reading only Geometry? yes|no|Integer|
