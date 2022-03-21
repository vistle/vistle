
# Thicken
transform lines to tubes or points to spheres

<svg width="50.599999999999994em" height="8.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="2.8em" width="5.06em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<rect x="0.7em" y="0.7999999999999998em" width="0.03333333333333333em" height="2.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="0.7em" y="0.7999999999999998em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="1.9em" y="0.8999999999999998em" class="text" ><tspan> (grid_in)</tspan></text>
<rect x="1.4em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="1.9em" y="1.7999999999999998em" width="0.03333333333333333em" height="1.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="1.9em" y="1.7999999999999998em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="1.9em" class="text" ><tspan> (data_in)</tspan></text>
<text x="0.2em" y="4.65em" class="moduleName" >Thicken</text><rect x="0.2em" y="4.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="0.7em" y="5.8em" width="0.03333333333333333em" height="2.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="0.7em" y="7.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="1.9em" y="7.8999999999999995em" class="text" ><tspan> (grid_out)</tspan></text>
<rect x="1.4em" y="4.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="1.9em" y="5.8em" width="0.03333333333333333em" height="1.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="1.9em" y="6.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="6.8999999999999995em" class="text" ><tspan> (data_out)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|radius|radius or radius scale factor of tube/sphere|Float|
|sphere_scale|extra scale factor for spheres|Float|
|map_mode|mapping of data to tube diameter (DoNothing, Fixed, Radius, InvRadius, Surface, InvSurface, Volume, InvVolume)|Int|
|range|allowed radius range|Vector|
|start_style|cap style for initial tube segments (Open, Flat, Round, Arrow)|Int|
|connection_style|cap style for tube segment connections (Open, Flat, Round, Arrow)|Int|
|end_style|cap style for final tube segments (Open, Flat, Round, Arrow)|Int|
