
# IsoSurface
Extract surfaces with constant data level

## Input ports
|name|description|
|-|-|
|data_in||
|mapdata_in||


<svg width="475.99999999999994" height="210" >
<rect x="0" y="0" width="475.99999999999994" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="98.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>mapdata_in</title></rect>
<title>mapdata_in</title></rect><rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">IsoSurface</text></svg>

## Output ports
|name|description|
|-|-|
|data_out||


## Parameters
|name|description|type|
|-|-|-|
|isovalue|isovalue|Float|
|isopoint|isopoint|Vector|
|point_or_value|point or value interaction (PointPerTimestep, Value, PointInFirstStep)|Integer|
|processortype|processortype (Host, Device)|Integer|
|compute_normals|compute normals (structured grids only)|Integer|
