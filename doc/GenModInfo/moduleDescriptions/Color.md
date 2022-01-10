
# Color
Map scalar and vector data to colors

<svg width="1200" height="240" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="60" width="120" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="21.0" y="30" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="30" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="33.0" class="text" ><tspan> (data_in)</tspan></text>
<rect x="6.0" y="120" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="21.0" y="150" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="213.0" class="text" ><tspan> (data_out)</tspan></text>
<rect x="42.0" y="120" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>color_out</title></rect>
<rect x="57.0" y="150" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="183.0" class="text" ><tspan> (color_out)</tspan></text>
<text x="6.0" y="115.5" class="moduleName" >Color</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|min|minimum value of range to map|Float|
|max|maximum value of range to map|Float|
|constrain_range|constrain range for min/max to data|Int|
|center|center of colormap range|Float|
|center_absolute|absolute value for center|Int|
|range_compression|compression of range towards center|Float|
|opacity_factor|multiplier for opacity|Float|
|map|transfer function name (COVISE, Plasma, Inferno, Magma, CoolWarmBrewer, CoolWarm, Frosty, Dolomiti, Parula, Viridis, Cividis, Turbo, Blue_Light, Grays, Gray20, ANSYS, Star, ITSM, Rainbow, Earth, Topography, RainbowPale)|Int|
|steps|number of color map steps|Int|
|blend_with_material|use alpha for blending with diffuse material|Int|
|auto_range|compute range automatically|Int|
|preview|use preliminary colormap for showing preview when determining bounds|Int|
|nest|inset another color map|Int|
|auto_center|compute center of nested color map|Int|
|inset_relative|width and center of inset are relative to range|Int|
|inset_center|where to inset other color map (auto range: 0.5=middle)|Float|
|inset_width|range covered by inset color map (auto range: relative)|Float|
|inset_map|transfer function to inset (COVISE, Plasma, Inferno, Magma, CoolWarmBrewer, CoolWarm, Frosty, Dolomiti, Parula, Viridis, Cividis, Turbo, Blue_Light, Grays, Gray20, ANSYS, Star, ITSM, Rainbow, Earth, Topography, RainbowPale)|Int|
|inset_steps|number of color map steps for inset (0: as outer map)|Int|
|resolution|number of steps to compute|Int|
|inset_opacity_factor|multiplier for opacity of inset color|Float|
