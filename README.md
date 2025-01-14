[![Build status](https://github.com/vistle/vistle/workflows/CMake/badge.svg)](https://github.com/vistle/vistle/actions?query=workflow%3ACMake)


Vistle - A Modular Data-Parallel Visualization System
=====================================================

> **vistle**:
>	very smooth and elegant, nicely put together ([Urban Dictionary](https://www.urbandictionary.com/define.php?term=vistle))


License
-------

Vistle source code is licensed under the LGPL v2.1. See `LICENSE.txt` for details. This does not apply to the subdirectory `lib/3rdparty`.


Installation
------------

### Installing with [Spack](https://spack.io)

  Add the [HLRS Vis Spack repository](https://github.com/hlrs-vis/spack-hlrs-vis) to your installation of Spack:

      git clone https://github.com/hlrs-vis/spack-hlrs-vis
      spack repo add spack-hlrs-vis

  Then you can install Vistle with this command:

      spack install vistle


### macOS with [Homebrew](https://brew.sh)

  Install most of Vistle's dependencies by invoking `brew bundle` within Vistle's root source directory. You can also install Vistle with

      brew install hlrs-vis/tap/vistle


### Installing From Source

#### Build Requirements

- **C++ compiler**:
  support for C++14 (ISO/IEC 14882:2014) is required,
  known good compilers:
    - GCC (8-14)
    - Clang (Xcode 14, 15)
    - Microsoft Visual Studio 2022

- **CMake**:
  at least 3.10, on Windows 3.15

- **MPI**:
  Microsoft MPI, Open MPI, MPICH and MVAPICH2 has been used successfully.

- **Boost**:
  At least 1.66.00 is required.
     Note:
     - in order to switch MPI implementations without requiring a recompilation of Boost, we compile Boost.MPI together with Vistle

- Botan:
  [Crypto and TLS for Modern C++](https://botan.randombit.net/) is used for verifying HMACs during connection establishment,
  version 2 or 3 should work

- **Python**:
  for interpreting Vistle workflow scripts (.vsl), Python 3 should work.

- **COVISE/OpenCOVER**:
  OpenCOVER or a version of COVISE including OpenCOVER compiled by you is necessary, get it from
  [GitHub](https://github.com/hlrs-vis/covise), needed for the COVER module and COVER plug-ins
  You can shorten the build process and cut down on dependencies by [building only OpenCOVER](https://github.com/hlrs-vis/covise#building-only-opencover).
  In addition you should set `COVISEDESTDIR` to a location where compiled COVER plug-ins should go.
      Hints:
      - The COVISE repository contains further information on how to build dependencies on Linux ([README-Building-deps-linux.txt](https://raw.githubusercontent.com/hlrs-vis/covise/master/README-Building-deps-linux.txt)) and Windows ([README-Building-deps.txt](https://raw.githubusercontent.com/hlrs-vis/covise/master/README-Building-deps.txt)).

- **OpenSceneGraph**:
  the version of OpenSceneGraph that was used for compiling OpenCOVER

- **Qt**:
  The Qt 6 libraries `Qt6Core`, `Qt6Widgets` and `Qt6Gui` are required by the graphical user interface,
  alternatively also Qt 5 can be used.

You can set the environment variable `EXTERNLIBS` to a directory where Vistle should search for 3rd party libraries. It will search in e.g. `$EXTERNLIBS/boost`, if CMake is looking for `Boost`.


#### Getting Vistle

Getting Vistle is as easy as

      git clone --recursive https://github.com/vistle/vistle.git

If you need to update an existing working copy, then change to its directory and issue the following commands:

      git submodule sync --recursive
      git submodule update --init --recursive

#### Building Vistle

Create a subdirectory for building, change to it, and invoke CMake:

      cmake ..

Then build with your build tool, e.g.:

      make -j20


Invoking Vistle
---------------

### Environment Set-Up

Vistle modules are run on clusters via MPI. You have to configure how they have to be spawned by providing a script named `spawn_vistle.sh` (or `spawn_vistle.bat` on Windows) somewhere in your `PATH`. It could be as simple as

      #! /usr/bin/env bash
      mpirun "$@"

But it also might require invoking your batch system. An example is provided in `vistle/bin`.

For COVER to find its plug-ins, you should add the directory from `COVISEDESTDIR` to `COVISE_PATH`.

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

- `cmake`:
  CMake modules

- `contrib/scripts`:
  support scripts for building and running Vistle

- `lib/3rdparty`:
  3rd party source code

- `lib/vistle`:
  Vistle libraries source code

    - `lib/vistle/util`: support code
    - `lib/vistle/core`: Vistle core data structures
    - `lib/vistle/userinterface`: common library for user interfaces
    - `lib/vistle/net`: library for Vistle network communication
    - `lib/vistle/control`: Python code for controlling a Vistle session
    - `lib/vistle/module`: base class and support code for visualization algorithm modules
    - `lib/vistle/renderer`: base class and support code for render modules
    - `lib/vistle/rhr`: library for remote hybrid rendering servers and clients

- `app`:
  Vistle applications

    - `app/hub`: Vistle session controller
    - `app/gui`: graphical user interface

- `module`:
  visualization algorithm modules and base library

    - `module/general`: modules useful to a wider audience
    - `module/test`: various debugging aids
    - `module/render`: renderer modules transforming geometry into pixels
        - `module/render/DisCOVERay`: a parallel remote hybrid rendering server based on [Embree](https://www.embree.org) (CPU ray-casting)
        - `module/render/OsgRenderer`: a parallel remote hybrid rendering server based on OpenSceneGraph (OpenGL)
        - `module/render/COVER`: wrap OpenCOVER as a render module
            - `module/render/COVER/plugin`: plug-ins for OpenCOVER, e.g. for connecting to Vistle
                - `module/render/COVER/plugin/RhrClient`: OpenCOVER remote hybrid rendering client plugin

Documentation
-------------

Some documentation on Vistle is posted to [vistle.io](https://vistle.io).
