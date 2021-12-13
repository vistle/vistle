
# ReadDyna3D
Read LS-Dyna3D ptf files



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
