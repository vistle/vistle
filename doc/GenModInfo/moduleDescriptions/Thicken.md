
# Thicken
Transform lines to tubes or points to spheres

<svg width="1517.9999999999998" height="270" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="90" width="151.79999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<rect x="21.0" y="30" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="30" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="33.0" class="text" ><tspan> (grid_in)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="57.0" y="60" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="60" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="63.0" class="text" ><tspan> (data_in)</tspan></text>
<rect x="6.0" y="150" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="21.0" y="180" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="240" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="243.0" class="text" ><tspan> (grid_out)</tspan></text>
<rect x="42.0" y="150" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="57.0" y="180" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="213.0" class="text" ><tspan> (data_out)</tspan></text>
<text x="6.0" y="145.5" class="moduleName" >Thicken</text></svg>

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
