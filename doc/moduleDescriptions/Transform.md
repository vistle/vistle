
# Transform
Apply transformation matrix to grid

## Input ports
|name|description|
|-|-|
|data_in|input data|


## Output ports
|name|description|
|-|-|
|data_out|output data|


## Parameters
|name|description|type|
|-|-|-|
|rotation_axis_angle|axis and angle of rotation|Vector|
|scale|scaling factors|Vector|
|translate|translation vector|Vector|
|keep_original|whether to keep input|Integer|
|repetitions|how often the transformation should be repeated|Integer|
|animation|animate repetitions (Keep, Deanimate, Animate, TimestepAsRepetitionCount)|Integer|
|mirror|enable mirror (Original, Mirror_X, Mirror_Y, Mirror_Z)|Integer|
