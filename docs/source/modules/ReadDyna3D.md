
# ReadDyna3D
read LS-Dyna3D ptf files

<svg width="68.0em" height="7.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="0.8em" width="6.8em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<text x="0.2em" y="2.6500000000000004em" class="moduleName" >ReadDyna3D</text><rect x="0.2em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="0.7em" y="3.8em" width="0.03333333333333333em" height="3.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="6.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="6.8999999999999995em" class="text" >grid<tspan> (grid_out)</tspan></text>
<rect x="1.4em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>data1_out</title></rect>
<rect x="1.9em" y="3.8em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="5.8999999999999995em" class="text" >vector data<tspan> (data1_out)</tspan></text>
<rect x="2.5999999999999996em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>data2_out</title></rect>
<rect x="3.0999999999999996em" y="3.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="3.0999999999999996em" y="4.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="4.3em" y="4.8999999999999995em" class="text" >scalar data<tspan> (data2_out)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Int|
|last_step|last timestep to read (-1: last)|Int|
|step_increment|number of steps to increment|Int|
|first_rank|rank for first partition of first timestep|Int|
|filename|Geometry file path|String|
|nodalDataType|Nodal results data to be read (No_Node_Data, Displacements, Velocities, Accelerations)|Int|
|elementDataType|Element results data to be read (No_Element_Data, Stress_Tensor, Plastic_Strain, Thickness)|Int|
|component|stress tensor component (Sx, Sy, Sz, Txy, Tyz, Txz, Pressure, Von_Mises)|Int|
|Selection|Number selection for parts|String|
|format|Format of LS-DYNA3D ptf-File (CADFEM, Original, Guess)|Int|
|byteswap|Perform Byteswapping (Off, On, Auto)|Int|
|OnlyGeometry|Reading only Geometry? yes|no|Int|
