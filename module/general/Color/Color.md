[headline]:<>
Color - map scalar and vector data to colors
============================================
[headline]:<>
[inputPorts]:<>
Input ports
-----------
|name|description|
|-|-|
|data_in||


[inputPorts]:<>
[outputPorts]:<>
Output ports
------------
|name|description|
|-|-|
|data_out||
|color_out||


[outputPorts]:<>
[parameters]:<>
Parameters
----------
|name|type|description|
|-|-|-|
|min|Float|minimum value of range to map|
|max|Float|maximum value of range to map|
|constrain_range|Int|constrain range for min/max to data|
|center|Float|center of colormap range|
|center_absolute|Int|absolute value for center|
|range_compression|Float|compression of range towards center|
|opacity_factor|Float|multiplier for opacity|
|map|Int|transfer function name|
|steps|Int|number of color map steps|
|blend_with_material|Int|use alpha for blending with diffuse material|
|auto_range|Int|compute range automatically|
|preview|Int|use preliminary colormap for showing preview when determining bounds|
|nest|Int|inset another color map|
|auto_center|Int|compute center of nested color map|
|inset_relative|Int|width and center of inset are relative to range|
|inset_center|Float|where to inset other color map (auto range: 0.5=middle)|
|inset_width|Float|range covered by inset color map (auto range: relative)|
|inset_map|Int|transfer function to inset|
|inset_steps|Int|number of color map steps for inset (0: as outer map)|
|resolution|Int|number of steps to compute|
|inset_opacity_factor|Float|multiplier for opacity of inset color|

[parameters]:<>
