
# BoundingBox
compute bounding boxes

## Purpose

The bounding box can be used as a rough guide to the interesting areas of the dat set. The box can
also provide visual clues that help with orientation in 3D space. Showing bounding boxes for
individual blocks will allow you to assess the partitioning of your data. The numerical values can
be used to craft input for modules requiring coordinates as parameter input.

## Ports


<svg width="73.8em" height="6.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="1.8em" width="7.38em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="1.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<rect x="0.7em" y="0.8em" width="0.03333333333333333em" height="1.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="0.7em" y="0.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="1.9em" y="0.9em" class="text" >input data<tspan> (grid_in)</tspan></text>
<text x="0.2em" y="3.6500000000000004em" class="moduleName" >BoundingBox</text><rect x="0.2em" y="3.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="0.7em" y="4.8em" width="0.03333333333333333em" height="1.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="0.7em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="1.9em" y="5.8999999999999995em" class="text" >bounding box<tspan> (grid_out)</tspan></text>
</svg>

The BoundingBox module takes its geometry input from `grid_in` and finds global minimum and maximum
values for its coordinates. The result of this process can be seen in its parameter window as the
values of the `min` and `max` parameters. The location of the extremal values are recorded in its
other output parameters. It also provides a tight axis-aligned cuboid around the domain of the data
at the `grid_out` output.


## Parameters
|name|description|type|
|-|-|-|
|per_block|create bounding box for each block|Int|
|min|output parameter: minimum|Vector|
|max|output parameter: maximum|Vector|
|min_block|output parameter: block numbers containing minimum|IntVector|
|max_block|output parameter: block numbers containing maximum|IntVector|
|min_index|output parameter: indices of minimum|IntVector|
|max_index|output parameter: indices of maximum|IntVector|

By enabling `per_block`, it creates an enclosing cuboid for each input
block individually instead of for all blocks globally.

## Usage Examples

![]( ../../../module/general/BoundingBox/tiny-covise-net.png )
Workflow for reading scalar and vector fields and drawing a bounding box around the domain of the data

![]( ../../../module/general/BoundingBox/tiny-covise.png )
Rendering of visualization with a enclosing bounding box

## Related Modules

### Similar Modules

[Extrema](Extrema_link)
