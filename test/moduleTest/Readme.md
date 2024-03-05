## Create ref image hashes

- vistle_hash new command to create ref images
- add MODULE_CAN_CREATE_HASH to CMakeLists.txt of the desired module (others are skipped, shown in cmake config output)

## Requirements:
- covise (swig must be installed, to avoid '"NameError: name 'coGRMsg' is not defined")
- .vsl for desired module (+.vwp file)
- image hash lib in python (https://github.com/JohannesBuchner/imagehash)

## Cmake Pipeline
- CM=CMakeLists.txt 
add_module (ROOT_CM) > add_module2 (ROOT_CM) > add_image_hash (Test.cmake) > create_image_hash (VistleUtils.cmake)