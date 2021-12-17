
# Transform
Apply transformation matrix to grid

<svg width="1866.0" height="210" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="60" width="186.6" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="21.0" y="30" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="30" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="33.0" class="text" >input data<tspan> (data_in)</tspan></text>
<rect x="6.0" y="120" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="21.0" y="150" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="183.0" class="text" >output data<tspan> (data_out)</tspan></text>
<text x="6.0" y="115.5" class="moduleName" >Transform</text></svg>

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
