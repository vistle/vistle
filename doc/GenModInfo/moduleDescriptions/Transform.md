
# Transform
Apply transformation matrix to grid

## Input ports
|name|description|
|-|-|
|data_in|input data|


<svg width="186.6" height="90" >
<rect x="0" y="0" width="186.6" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">Transform</text></svg>

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
