
# IsoHeightSurface
Extract surface at constant height

## Input ports
|name|description|
|-|-|
|data_in||
|mapdata_in||


<svg width="308.4" height="90" >
<rect x="0" y="0" width="308.4" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="42.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>mapdata_in</title></rect>
<title>mapdata_in</title></rect><rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">IsoHeightSurface</text></svg>

## Output ports
|name|description|
|-|-|
|data_out||


## Parameters
|name|description|type|
|-|-|-|
|iso height|height above ground|Float|
|processortype|processortype (Host, Device)|Integer|
|compute_normals|compute normals (structured grids only)|Integer|
|heightmap|height map as geotif|String|
