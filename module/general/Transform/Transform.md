Transform
=========
apply transformation matrix to grid

Input ports
-----------
|name|description|
|-|-|
|data_in|input data|

Output ports
------------
|name|description|
|-|-|
|data_out|output data|

Parameters
----------
|name|type|description|
|-|-|-|
|rotation_axis_angle|Vector|axis and angle of rotation|
|scale|Vector|scaling factors|
|translate|Vector|translation vector|
|keep_original|Int|whether to keep input|
|repetitions|Int|how often the transformation should be repeated|
|animation|Int|animate repetitions|
|mirror|Int|enable mirror|
