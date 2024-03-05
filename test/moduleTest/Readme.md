## Create ref image hashes

- vistle_ref_hash new command to create ref images
- add MODULE_CAN_CREATE_HASH to CMakeLists.txt of the desired module (others are skipped, shown in cmake config output)
  **important:** set(MODULE_CAN_CREATE_HASH True) must appear **before** add_module

## Requirements:
- covise (swig must be installed, to avoid '"NameError: name 'coGRMsg' is not defined")
- .vsl for desired module (+.vwp file)
- image hash lib in python (https://github.com/JohannesBuchner/imagehash)

### How to Create Viewpoint (currently, will probably be made more user-friendly soon :) )
In Vistle:
- create map in vsl as usual (has to use COVER as Renderer)
In COVER: 
- load random file (format does not matter!) with "File" tab, note that the name of the file will be the name of the .vwp file and that it will be saved in the **same folder** as this input file (*recommended:* load a file that is inside the directory of the module, e.g. "module/test/Gendat/Gendat.h")
- execute map, move camera to where you want
- then in Cover go to "Viewpoints" tab -> "Create Viewpoint" 
- then still in "Viewpoints" tab -> "Edit viewpoints" -> "save viewpoints"

## Cmake Pipeline
- CM=CMakeLists.txt 
add_module (ROOT_CM) > add_module2 (ROOT_CM) > add_image_hash (Test.cmake) > create_image_hash (VistleUtils.cmake)


## Todo:

[ ] test if hashes are the same on different operating systems
