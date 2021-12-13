
# Color
Map scalar and vector data to colors

## Input ports
|name|description|
|-|-|
|data_in||


## Output ports
|name|description|
|-|-|
|data_out||
|color_out||


## Parameters
|name|description|type|
|-|-|-|
|min|minimum value of range to map|Float|
|max|maximum value of range to map|Float|
|constrain_range|constrain range for min/max to data|Integer|
|center|center of colormap range|Float|
|center_absolute|absolute value for center|Integer|
|range_compression|compression of range towards center|Float|
|opacity_factor|multiplier for opacity|Float|
|map|transfer function name (COVISE, Plasma, Inferno, Magma, CoolWarmBrewer, CoolWarm, Frosty, Dolomiti, Parula, Viridis, Cividis, Turbo, Blue_Light, Grays, Gray20, ANSYS, Star, ITSM, Rainbow, Earth, Topography, RainbowPale)|Integer|
|steps|number of color map steps|Integer|
|blend_with_material|use alpha for blending with diffuse material|Integer|
|auto_range|compute range automatically|Integer|
|preview|use preliminary colormap for showing preview when determining bounds|Integer|
|nest|inset another color map|Integer|
|auto_center|compute center of nested color map|Integer|
|inset_relative|width and center of inset are relative to range|Integer|
|inset_center|where to inset other color map (auto range: 0.5=middle)|Float|
|inset_width|range covered by inset color map (auto range: relative)|Float|
|inset_map|transfer function to inset (COVISE, Plasma, Inferno, Magma, CoolWarmBrewer, CoolWarm, Frosty, Dolomiti, Parula, Viridis, Cividis, Turbo, Blue_Light, Grays, Gray20, ANSYS, Star, ITSM, Rainbow, Earth, Topography, RainbowPale)|Integer|
|inset_steps|number of color map steps for inset (0: as outer map)|Integer|
|resolution|number of steps to compute|Integer|
|inset_opacity_factor|multiplier for opacity of inset color|Float|
