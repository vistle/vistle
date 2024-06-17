# How to generate Snapshots for the documentation using CMake

To keep the size of the repository as small as possible, screenshots should be generated automatically through Vistle's documentation pipeline if possible. Therefore Vistle provides some CMake macros that manage the generation. This allows recreation of the images if the dependencies change. These dependencies are the Vistle workflow file itself and a module or other CMake tarket whose functionality should be shown in the snapshot (for now only one target dependency is supported).

## Snapshots of the workflow and its result

The CMake macro `generate_cover_snapshot(targetname network_file add_workflow_snapshot ...)` generates both snapshots of the workflow (provided that `add_workflow_snapshot` is TRUE) as well as the result as rendered in cover. It uses *network_file*.vsl and the viewpoint file *network_file*.vwp in the CMakeLists.txt's directory to generate the images *network_file*_workflow.png and *network_file*_result.png in the target's build directory.

The viewpoint file can be created by using the ViewPoint plugin.

Further adaptions to the cover presentation can be done using coGRMsgs that can be send to COVER using Vislte's pythen interface e.g.:

```python
    sendCoverMessage(coGRColorBarPluginMsg(coGRColorBarPluginMsg.ShowColormap))
```

will show all color tables.

To automatically link and use the images in a module's documentation input file use the example tag:

    [example]:<network_file>
