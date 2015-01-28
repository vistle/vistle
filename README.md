Visualization Testing Laboratory for Exascale Computing (Vistle)
================================================================

A modular data-parallel visualization system.

> **vistle**:
>	very smooth and elegant, nicely put together ([Urban Dictionary](http://www.urbandictionary.com/define.php?term=vistle))


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

  Known good compilers:
  - GCC (4.6, 4.8, 4.9)
  - Clang (Xcode 5.0–6.1)
  - Intel (14.0.2 - but use Boost 1.52 and not on Cray, 13.1.3 with GCC 4.6.3)
  
  Known bad compilers:
  - GCC 4.4 (not enough C++11)
  - PGI 13.10 (no atomics for boost:interprocess)
  - Cray CC 8.2.3 (crashes on IceT, not enough C++11)

- **CMake**:
  at least 2.8

- **MPI**:
  Open MPI, MPICH and MVAPCH2 has been used successfully.

- **Boost**:
  Build boost with the following options:

         b2 --with-filesystem --with-iostreams --with-python \
             --with-serialization --with-system --with-thread \
             --with-regex --with-chrono --with-date_time \
             --with-program_options
     Notes:
     - in order to switch MPI implementations without requiring a recompilation of boost, we compile Boost.MPI together with Vistle
     - Intel compiler (at least 14.0.2) does not work with 1.55 because of missing `std::nullptr_t`, use 1.52

- **Python**:
  for interpreting Vistle scripts (.vsl), only tested with Python 2.6 and 2.7

- **Readline library**:
  history and line editing for command line interface

- **COVISE**:
  a version of COVISE compiled by you is necessary, get it from
  [GitHub](https://github.com/hlrs-vis/covise), needed for:
  
  - ReadCovise module
  - COVER plug-in
  - ray casting render module

- **OpenSceneGraph**:
  the version of OpenSceneGraph that was used for compiling COVER

- **Qt**:
  The Qt 5 libraries `Qt5Core`, `Qt5Widgets` and `Qt5Gui` are required by the graphical user interface.

Building Vistle
---------------

Create a subdirectory for building, change to it, and invoke cmake:

      cmake ..

Then build with your build tool, e.g.:

      make -j20

Invoking Vistle
---------------

      vistle [--batch|--gui|--tui] [scriptfile]

Options:

* `-b`|`--batch`:
  do not start a user interface

* `-g`|`--gui` (default):
  start a graphical user interface on rank 0

* `-t`|`--tui`:
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

- `vistle/cover`:
  plugins for OpenCOVER, e.g. for connecting to Vistle
