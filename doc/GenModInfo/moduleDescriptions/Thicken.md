
# Thicken
Transform lines to tubes or points to spheres

## Input ports
|name|description|
|-|-|
|grid_in||
|data_in||


<svg width="151.79999999999998" height="90" >
<rect x="0" y="0" width="151.79999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<title>grid_in</title></rect><rect x="42.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">Thicken</text></svg>

## Output ports
|name|description|
|-|-|
|grid_out||
|data_out||


## Parameters
|name|description|type|
|-|-|-|
|radius|radius or radius scale factor of tube/sphere|Float|
|sphere_scale|extra scale factor for spheres|Float|
|map_mode|mapping of data to tube diameter (DoNothing, Fixed, Radius, InvRadius, Surface, InvSurface, Volume, InvVolume)|Integer|
|range|allowed radius range|Vector|
|start_style|cap style for initial tube segments (Open, Flat, Round, Arrow)|Integer|
|connection_style|cap style for tube segment connections (Open, Flat, Round, Arrow)|Integer|
|end_style|cap style for final tube segments (Open, Flat, Round, Arrow)|Integer|
