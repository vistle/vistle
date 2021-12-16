
# Transform
Apply transformation matrix to grid

## Input ports
|name|description|
|-|-|
|data_in|input data|


<svg width="435.4" height="210" >
<rect x="0" y="0" width="435.4" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">Transform</text></svg>

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
