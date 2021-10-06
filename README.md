[![Build status](https://github.com/vistle/vistle/workflows/CMake/badge.svg)](https://github.com/vistle/vistle/actions?query=workflow%3ACMake)

Vistle - A Modular Data-Parallel Visualization System
=====================================================

> **vistle**:
>	very smooth and elegant, nicely put together ([Urban Dictionary](http://www.urbandictionary.com/define.php?term=vistle))


License
-------

Vistle source code is licensed under the LGPL v2.1. See `lgpl-2.1.txt` for
details. This does not apply to the subdirectory `3rdparty`.


Getting Vistle
--------------

Getting Vistle is as easy as

		git clone --recursive https://github.com/vistle/vistle.git
      
If you need to update an existing working copy, then change to its directory and issue the following commands:
      
	  git submodule sync
	  git submodule update --init --recursive


Build Requirements
------------------

- **C++ compiler**:
  support for C++14 (ISO/IEC 14882:2014) is required

  Known good compilers:
  - GCC (5.3, 8, 9)
  - Clang (Xcode 11)
  - Microsoft Visual Studio 2017 (15.1)
  
- **CMake**:
  at least 3.3

- **MPI**:
  Microsoft MPI, Open MPI, MPICH and MVAPICH2 has been used successfully.

- **Boost**:
  At least 1.59.00 is required.
  Build boost with the following options:

         b2 --with-filesystem --with-iostreams --with-python \
             --with-serialization --with-system --with-thread \
             --with-date_time --with-chrono --with-regex \
             --with-program_options
     Notes:

     - in order to switch MPI implementations without requiring a recompilation of boost, we compile Boost.MPI together with Vistle

- Botan:
  [Crypto and TLS for Modern C++](https://botan.randombit.net/) is used for verifying HMACs during connection establishment,
  version 1 or 2 should work

- **Python**:
  for interpreting Vistle scripts (.vsl), Python 2.7 and 3 should work.

- **COVISE/OpenCOVER**:
  OpenCOVER or a version of COVISE including OpenCOVER compiled by you is necessary, get it from
  [GitHub](https://github.com/hlrs-vis/covise), needed for the COVER module and COVER plug-ins
 
  You can shorten the build process and cut down on dependencies by [building only OpenCOVER](https://github.com/hlrs-vis/covise#building-only-opencover).

  In addition you should set `COVISEDESTDIR` to a location where compiled COVER plug-ins
  should go.

  Hint: The COVISE repository contains further information on how to build dependencies
  on Linux (README-Building-deps-linux.txt) and Windows (README-Building-deps.txt).

- **OpenSceneGraph**:
  the version of OpenSceneGraph that was used for compiling OpenCOVER

- **Qt**:
  The Qt 5 libraries `Qt5Core`, `Qt5Widgets` and `Qt5Gui` are required by the graphical user interface.

You can set the environment variable `EXTERNLIBS` to a directory where Vistle
should search for 3rd party libraries.
It will search in e.g. `$EXTERNLIBS/boost` if CMake is looking for `Boost`.


### macOS with [Homebrew](https://brew.sh)

  Install most of Vistle's dependencies by invoking `brew bundle` within
  Vistle's root source directory. You can also install Vistle with

      brew install hlrs-vis/tap/vistle


### Installing with [Spack](https://spack.io)

  Add the [HLRS Vis Spack repository](https://github.com/hlrs-vis/spack-hlrs-vis) to your installation of Spack:

      git clone https://github.com/hlrs-vis/spack-hlrs-vis
      spack repo add spack-hlrs-vis

  Then you can install Vistle with this command:

      spack install vistle


Building Vistle
---------------

Create a subdirectory for building, change to it, and invoke CMake:

      cmake ..

Then build with your build tool, e.g.:

      make -j20


Invoking Vistle
---------------

### Environment Set-Up

Vistle modules are run on clusters via MPI. You have to configure how they
have to be spawned by providing a script named `spawn_vistle.sh` (or `spawn_vistle.bat`
on Windows) somewhere in your `PATH`. It could be as simple as

      #! /usr/bin/env bash
      mpirun "$@"

But it also might require invoking your batch system.
An example is provided in `vistle/bin`.

For COVER to find its plug-ins, you should add the directory from
`COVISEDESTDIR` to `COVISE_PATH`.

### Command Line Switches

Synopsis:

      vistle [--batch|--gui|--tui] [scriptfile]

Options:

* `-b`|`--batch`:
  do not start a user interface

* `-g`|`--gui` (default):
  start a graphical user interface on rank 0

* `-t`|`--tui`:
  start a command line user interface on rank 0

You can connect a user interface to a running Vistle session later on, e.g.:

      vistle_gui localhost 31093


Source Code Organization
------------------------

- `.../cmake`:
  CMake modules

- `scripts`:
  support scripts for building Vistle

- `3rdparty`:
  3rd party source code

- `vistle`:
  Vistle source code

    - `vistle/util`: support code
    - `vistle/core`: Vistle core data structures
    - `vistle/userinterface`: common library for user interfaces
    - `vistle/rhr`: library for remote hybrid rendering servers and clients
    - `vistle/control`: Python code for controlling a Vistle session
    - `vistle/hub`: Vistle session controller
    - `vistle/gui`: graphical user interface
    - `vistle/module`: visualization algorithm modules and base library
        - `vistle/module/general`:
        - `vistle/module/test`: various debugging aids
    - `vistle/renderer`: renderer modules transforming geometry into pixels
        - `vistle/renderer/DisCOVERay`: a parallel remote hybrid rendering server based on Embree (CPU ray-casting)
        - `vistle/renderer/OsgRenderer`: a parallel remote hybrid rendering server based on OpenSceneGraph (OpenGL)
    - `vistle/cover`: plugins for OpenCOVER, e.g. for connecting to Vistle
        - `vistle/cover/RhrClient`: OpenCOVER remote hybrid rendering client plugin

Documentation
-------------

Some documentation on Vistle is posted to
[vistle.io](https://vistle.io).
