[headline]:<>

## Purpose

The bounding box can be used as a rough guide to the interesting areas of the data set. The box can
also provide visual clues that help with orientation in 3D space. Showing bounding boxes for
individual blocks will allow you to assess the partitioning of your data. The numerical values can
be used to craft input for modules requiring coordinates as parameter input.

## Ports

[moduleHtml]:<>

The BoundingBox module takes its geometry input from `grid_in` and finds global minimum and maximum
values for its coordinates. The result of this process can be seen in its parameter window as the
values of the `min` and `max` parameters. The location of the extremal values are recorded in its
other output parameters. It also provides a tight axis-aligned cuboid around the domain of the data
at the `grid_out` output.

[parameters]:<>

By enabling `per_block`, it creates an enclosing cuboid for each input
block individually instead of for all blocks globally.

## Usage Examples

[example]:<tiny-covise>


## Related Modules

### Similar Modules

[Extrema]()
