# How to generate Snapnshots for the documentation using CMake

To keep the size of the repository as small as possible, screenshots should be generated automatically through Vistle's documentation pipeline if possible. Therefore Vistle provides some CMake macros that manage the generation. This allows recreation of the images if the dependencies change. These dependencies are the Vistle workflow file itself and a module or other CMake tarket whose functionality should be shown in the snapshot (for now only one target dependency is supported).

## Snapnshot of the Vistle workflow

The CMake macro **generate_network_snapshot(targetname network_file)** will create the snapshot named *network_file*_workflow.png in the target's build directory.
It requires the file with the network *network_file*.vsl to be in the directory of the CMakeLists.txt

## Snapnshot of the result

The macro **generate_snapshots(targetname network_file)** generates both snapshots of the workflow as well as the result as rendered in cover. It uses *network_file*.vsl and the viewpoint file *network_file*.vwp in the CMakeLists.txt's directory to generate the images *network_file*_workflow.png and *network_file*_result.png in the target's build directory.

The viewpoint file can be created by using the ViewPoint plugin.

Further adaptions to the cover presentation can be done using coGRMsgs that can be send to COVER using Vislte's pythen interface e.g.:

```python
    sendCoverMessage(coGRColorBarPluginMsg(coGRColorBarPluginMsg.ShowColormap))
```

will show all color tables.

To automatically link and use the images in a module's documentation input file use the example tag:

    [example]:<network_file>
