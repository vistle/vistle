
# ReadDyna3D
Read ls-dyna3d ptf files



<svg width="475.99999999999994" height="210" >
<rect x="0" y="0" width="475.99999999999994" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="98.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data1_out</title></rect>
<rect x="182.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data2_out</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">ReadDyna3D</text></svg>

## Output ports
|name|description|
|-|-|
|grid_out|grid|
|data1_out|vector data|
|data2_out|scalar data|


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
