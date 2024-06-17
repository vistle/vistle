## Create ref image hashes

- vistle_ref_hash new command to create ref images
- add CREATE_IMAGE_HASH to CMakeLists.txt of the desired module (others are skipped, shown in cmake config output)
  **important:** `set(CREATE_IMAGE_HASH True)` must appear **before** `add_module`

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
### vistle_ref_hash
- CM=CMakeLists.txt 
add_module (ROOT_CM) > add_module2 (ROOT_CM) > add_image_hash (Test.cmake) > create_image_hash (Test.cmake)

### ctest
moduleTest/CM > utils/createAndCompareHash.sh > CMAKE_SOURCE_DIR/doc/resultSnapShot.py + utils/createImageHash.py

## Module tests
Compile Vistle:

```
cd build
cmake -G Ninja ..
ninja
```
### How to create ref hashes
The reference screenshots are stored in the root directories of each module, e.g., ```module/map/DomainSurface```, with the suffix ```_result.png```.
```
cd build
ninja vistle_ref_hash
```

Hashes will be stored in moduleTest/utils/refImagHash.json, e.g.:

```JSON
{
    "module/map/DomainSurface/domain.vsl": "003e7e7e7e7e3c00",
    "module/test/Gendat/Isosurface.vsl": "001c3e3e3e3c0000"
}
```

### How to run tests
```
cd build
ctest
```

In case a test fails, you can check the screenshot in ```build/test/module_tmp```.

**Known errors:**

If you get the error `No tests were found!!!`, try the following:

```
cd build
ninja vistle_ref_hash # populate refImageHash.json
cmake -G Ninja ..
ctest
```
 Typically, you get this error when you configure the project while the refImageHash.json file is still empty. This is because CMake create a test for each entry in the reference file.
 
 This issue will be resolved in the future once the official reference JSON file is pushed to git.

## Todo:
[x] refactor .cmake files in `cmake`-folder (only those concerning docs/testing)

[ ] refactor `test/moduleTest`

[ ] refactor root `CMakeLists.txt` (only those concerning docs/testing)

[ ] create more reference maps

[ ] push refImageHash.json to git (serves as reference point for the tests)

[ ] allow multiple screenshots per module
