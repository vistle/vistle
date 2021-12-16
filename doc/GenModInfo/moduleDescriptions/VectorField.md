
# VectorField
Create lines from mapped vector data

## Input ports
|name|description|
|-|-|
|grid_in||
|data_in||


<svg width="516.5999999999999" height="210" >
<rect x="0" y="0" width="516.5999999999999" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<title>grid_in</title></rect><rect x="98.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">VectorField</text></svg>

## Output ports
|name|description|
|-|-|
|grid_out||


## Parameters
|name|description|type|
|-|-|-|
|scale|scale factor for vector length|Float|
|attachment_point|where to attach line to carrying point (Bottom, Middle, Top)|Integer|
|range|allowed length range (before scaling)|Vector|
