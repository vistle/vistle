
# Tubes
Transform lines to tubes

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
|radius|radius or radius scale factor of tube|Float|
|map_mode|mapping of data to tube diameter (Fixed, Radius, CrossSection, InvRadius, InvCrossSection)|Integer|
|range|allowed radius range|Vector|
|start_style|cap style for initial segments (Open, Flat, Round, Arrow)|Integer|
|connection_style|cap style for segment connections (Open, Flat, Round, Arrow)|Integer|
|end_style|cap style for final segments (Open, Flat, Round, Arrow)|Integer|
