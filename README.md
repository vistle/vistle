Visualization Testing Laboratory for Exascale Computing (Vistle)
================================================================

A modular data-parallel visualization system.


License
-------

Vistle source code is licensed under the LGPL v2.1. See lgpl-2.1.txt for
details. This does not apply to the subdirectory 3rdparty.


Build Requirements
------------------

C++ compiler
: support for C++11 (ISO/IEC 14882:2011) is required

CMake
: at least 2.8

MPI
:

Boost
: Build boost with the following options:

         b2 --with-filesystem --with-iostreams --with-python --with-serialization --with-system --with-thread

COVISE
: a compiled source code distribution or a developer installation is required for the COVER plugin,
   get it from [HLRS](http://www.hlrs.de/organization/av/vis/covise/support/download/)

OpenSceneGraph
: the version of OpenSceneGraph that was used for compiling COVER

Python
: for interpreting Vistle scripts (.vsl)

Readline library
: history and line editing for command line interface



Building Vistle
---------------

create a subdirectory for building, change to it and invoke cmake:

      cmake ..



Invoking Vistle
---------------

      vistle [-batch|-noui|-gui|-tui] [scriptfile]

Options:

-batch|-noui
: do not start a user interface

-gui (default)
: start a graphical user interface on rank 0

-tui
: start a command line user interface on rank 0

You can connect a user interface to a running Vistle session later on:

      gui localhost 31093



Source Code Organization
------------------------

.../cmake
: cmake modules

srcipts
: support scripts for building Vistle

3rdparty
: 3rd party source code

vistle
: Vistle source code

vistle/util
: support code

vistle/userinterface
: common library for user interfaces

vistle/gui
: graphical user interface

vistle/blower
: command line user interface (Python)

vistle/control
: library for controlling a Vistle session

vistle/vistle
: Vistle session controller

vistle/core
: Vistle core data structures

vistle/module
: visualization algorithm modules and base library

vistle/plugin
: COVER plugin for connecting to Vistle

