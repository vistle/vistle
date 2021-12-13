
# Thicken
Transform lines to tubes or points to spheres

## Input ports
|name|description|
|-|-|
|grid_in||
|data_in||


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
