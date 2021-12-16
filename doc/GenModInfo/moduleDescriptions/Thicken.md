
# Thicken
Transform lines to tubes or points to spheres

## Input ports
|name|description|
|-|-|
|grid_in||
|data_in||


<svg width="354.19999999999993" height="210" >
<rect x="0" y="0" width="354.19999999999993" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<title>grid_in</title></rect><rect x="98.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="98.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">Thicken</text></svg>

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
