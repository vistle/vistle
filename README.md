Visualization Testing Laboratory for Exascale Computing (Vistle)
================================================================

A modular data-parallel visualization system.


License
-------

Vistle source code is licensed under the LGPL v2.1. See `lgpl-2.1.txt` for
details. This does not apply to the subdirectory `3rdparty`.


Getting Vistle
--------------

Getting Vistle is as easy as

      git clone https://github.com/vistle/vistle.git
      cd vistle
      git submodule update --init


Build Requirements
------------------

- **C++ compiler**:
  support for C++11 (ISO/IEC 14882:2011) is required

- **CMake**:
  at least 2.8

- **MPI**:
  only tested with Open MPI (`MPI_Comm_spawn_multiple` behavior is particularly critical – Vistle relies on controlling the host on which processes are started)

- **Boost**:
  Build boost with the following options:

         b2 --with-filesystem --with-iostreams --with-python --with-serialization --with-system --with-thread

     Note: in order to switch MPI implementations without requiring a recompilation of boost, we compile Boost.MPI together with Vistle.

- **Python**:
  for interpreting Vistle scripts (.vsl), only tested with Python 2.6 and 2.7

- **Readline library**:
  history and line editing for command line interface

- **COVISE**:
  a compiled source code distribution or a developer installation is required for the COVER plugin and the ReadCovise module,
  get it from [HLRS](http://www.hlrs.de/organization/av/vis/covise/support/download/)

- **OpenSceneGraph**:
  the version of OpenSceneGraph that was used for compiling COVER

- **Qt**:
  Qt 5 is required by the graphical user interface


Building Vistle
---------------

Create a subdirectory for building, change to it, and invoke cmake:

      cmake ..

Then build with your build tool, e.g.:

      make -j20

Invoking Vistle
---------------

      vistle [-batch|-noui|-gui|-tui] [scriptfile]

Options:

* `-batch`|`-noui`:
  do not start a user interface

* `-gui` (default):
  start a graphical user interface on rank 0

* `-tui`:
  start a command line user interface on rank 0

You can connect a user interface to a running Vistle session later on:

      vistle_gui localhost 31093



Source Code Organization
------------------------

- `.../cmake`:
  cmake modules

- `scripts`:
  support scripts for building Vistle

- `3rdparty`:
  3rd party source code

- `vistle`:
  Vistle source code

- `vistle/util`:
  support code

- `vistle/userinterface`:
  common library for user interfaces

- `vistle/gui`:
  graphical user interface

- `vistle/blower`:
  command line user interface (Python)

- `vistle/control`:
  library for controlling a Vistle session

- `vistle/vistle`:
  Vistle session controller

- `vistle/core`:
  Vistle core data structures

- `vistle/module`:
  visualization algorithm modules and base library

- `vistle/plugin`:
  COVER plugin for connecting to Vistle

