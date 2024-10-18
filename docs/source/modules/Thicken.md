
# Thicken
transform lines to tubes or points to spheres

## Purpose

The **Thicken** module can be used to expand lines into tubes or points into spheres. The radius of the resulting tube/sphere can be set to a fixed value or be dependent of a given scalar value.

## Ports

<svg width="50.599999999999994em" height="8.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="2.8em" width="5.06em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<rect x="0.7em" y="0.7999999999999998em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="0.7999999999999998em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="0.8999999999999998em" class="text" >lines or points with scalar data for radius<tspan> (grid_in)</tspan></text>
<rect x="1.4em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="1.9em" y="1.7999999999999998em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="1.7999999999999998em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="1.9em" class="text" >mapped data<tspan> (data_in)</tspan></text>
<text x="0.2em" y="4.65em" class="moduleName" >Thicken</text><rect x="0.2em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="0.7em" y="5.8em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="7.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="7.8999999999999995em" class="text" >tubes or spheres<tspan> (grid_out)</tspan></text>
<rect x="1.4em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="1.9em" y="5.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="6.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="6.8999999999999995em" class="text" >tubes or spheres with mapped data<tspan> (data_out)</tspan></text>
</svg>

The first input port `grid_in` is required and expects lines or points which the module will expand to tubes or spheres. The module automatically detects whether lines or spheres are provided.
    The second input port `data_in` is optional and takes a data field that can be used to calculate the radii of these tubes/spheres.

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

There are different modes for setting the radius of the resulting geometric bodies which can be selected with `map_mode` in the parameter window. Additionally, the `radius` and `sphere scale` parameters can be altered, to fine-tune the resulting radius and a `range` can be given for clipping the values of the radii.

### Map modes:
- **DoNothing:** Does not change the geometry. However, if `data_in` is not empty, the mapped data is added to `grid_in` and returned through the `data_out` port.
- **Fixed:** Sets the radius to `radius`. If the input grid is a spheres object, it additionally multiplies the value `sphere scale` to the radius. We will refer to this value as *Fixed-scale* in the following.
- **Radius or InvRadius**: Maps the data values (or its reciprocal, respectively) provided through `data_in` (multiplied with *Fixed-scale*) to the radii of each data point/line. If the data is three-dimensional, the L2-norm is calculated.
- **Surface or InvSurface**: Like **Radius or InvRadius**, respectively, but before **Fixed-Scale** is multiplied to the radius, the square root of the values in `data_in`is calculated.
- **Volume or InvVolume**: Like **Surface or InvSurface**, respectively, but instead of the square root, the cube root is calculated.

## Usage Examples

<figure float="left">
    <img src="../../../../module/geometry/Thicken/thickenWorkflow.png" width=300/>
    <img src="../../../../module/geometry/Thicken/thickenExampleResult.png" width=350/>
    <figcaption>Fig.1. Example workflow using Thicken (left) and step-by-step results (right). On the top-right, a scalar data set is shown as points which can only be seen by zooming in considerably. In the center, the same data set is shown as spheres. At the bottom, the vector field has been added, after expanding its lines into tubes.</figcaption>
</figure>

In this example, the scalar data field of a data set is first transformed into points via [ToPoints](ToPoints.md) (see top-right image) and then thickened into spheres (see image in the center of the right image). The radii of the spheres depend on the scalar data field.
**Note:** To avoid rendering errors, it is important to pass the output to the [ToTriangles](ToTriangles.md) module whose `transform spheres` parameter has been **enabled**.

Moreover, three-dimensional input data is transformed into a vector field through the [VectorField](VectorField.md) module and the resulting lines are expanded into tubes (see bottom of the right image).

## Related Modules

### Often Used With

- [ToTriangles](ToTriangles.md)
- [ToPoints](ToPoints.md)
- [VectorField](VectorField.md)
