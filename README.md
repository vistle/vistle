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

      git clone https://github.com/vistle/vistle.git --recursive


Build Requirements
------------------

- **C++ compiler**:
  support for C++11 (ISO/IEC 14882:2011) is required

  Known good compilers:
  - GCC (4.8, 4.9, 5, 6, 7)
  - Clang (Xcode 5.0–8.3)
  - Microsoft Visual Studio 2015 (14.0)
  
  Known bad compilers:
  - GCC 4.4 (not enough C++11)
  - PGI 13.10 (no atomics for boost:interprocess)
  - Cray CC 8.2.3 (crashes on IceT, not enough C++11)

- **CMake**:
  at least 3.0

- **MPI**:
  Microsoft MPI, Open MPI, MPICH and MVAPCH2 has been used successfully.

- **Boost**:
  Build boost with the following options:

         b2 --with-filesystem --with-iostreams --with-python \
             --with-serialization --with-system --with-thread \
             --with-date_time \
             --with-program_options
     Notes:

     - in order to switch MPI implementations without requiring a recompilation of boost, we compile Boost.MPI together with Vistle
     - Intel compiler (at least 14.0.2) does not work with 1.55 because of missing `std::nullptr_t`, use 1.52

- **Python**:
  for interpreting Vistle scripts (.vsl), Python 2.7 and 3 should work.

- **Readline library**:
  history and line editing for command line interface

- **COVISE/OpenCOVER**:
  a version of COVISE including OpenCOVER compiled by you is necessary, get it from
  [GitHub](https://github.com/hlrs-vis/covise), needed for:
  
    - ReadCovise module
    - COVER module and COVER plug-ins

  In addition you should set `COVISEDESTDIR` to a location where compiled COVER plug=ins
  should go.

- **OpenSceneGraph**:
  the version of OpenSceneGraph that was used for compiling OpenCOVER

- **Qt**:
  The Qt 5 libraries `Qt5Core`, `Qt5Widgets` and `Qt5Gui` are required by the graphical user interface.

You can set the environment variable `EXTERNLIBS` to a directory where Vistle
should search for 3rd party libraries.
It will search in e.g. `$EXTERNLIBS/boost` if CMake is looking for `Boost`.

### macOS with [Homebrew](https://brew.sh)

  Install most of Vistle's dependencies by invoking `brew bundle` within
  Vistle's root source directory.


Building Vistle
---------------

Create a subdirectory for building, change to it, and invoke CMake:

      cmake ..

Then build with your build tool, e.g.:

      make -j20

### Cray XC40 at HLRS (hazelhen.hww.de) [Hazel Hen](https://www.hlrs.de/en/systems/cray-xc40-hazel-hen)

Preparation:
- install a newer version of CMake (3.12 works), to some directory (e.g. ~/hazelhen)
- install tbb
- install other libraries as required: embree, libjpeg-turbo, zstd, lz4, snappy

Set up your environment (also required before using Vistle):

      export CRAYPE_LINK_TYPE=dynamic
      module swap PrgEnv-cray PrgEnv-gnu
      module load tools/vtk/8.1.0
      module load tools/boost/1.66.0

You most likely will want to install additional dependencies.

From your build directory, invoke CMake like this:

      cmake -DCMAKE_SYSTEM_NAME=CrayLinuxEnvironment ..


Invoking Vistle
---------------

### Environment Set-Up

Vistle modules are run on clusters via MPI. You have to configure how they
have to be spawned by providing a script named `spawn_vistle.sh` (or `spawn_vistle.bat`
on Windows) somewhere in your `PATH`. It could be as simple as

      #! /bin/bash
      mpirun "$@"

But it also might require invoking your batch system.

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

### Cray XC40 (Hazel Hen)

An example PBS script is provided in `scripts/vistle.pbs`. It reuses the
parameters supplied to the batch system. Something similiar to

      qsub -q test -lnodes=1,ppn=1,walltime=00:10:00 scripts/vistle.pbs

should get you started. This will start an instance of Vistle that connects
back to your work station.


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
    - `vistle/blower`: command line user interface (Python)
    - `vistle/module`: visualization algorithm modules and base library
        - `vistle/module/general`:
        - `vistle/module/test`: various debugging aids
    - `vistle/renderer`: renderer modules transforming geometry into pixels
        - `vistle/renderer/DisCOVERay`: a parallel remote hybrid rendering server based on Embree (CPU ray-casting)
        - `vistle/renderer/OsgRenderer`: a parallel remote hybrid rendering server based on OpenSceneGraph (OpenGL)
    - `vistle/cover`: plugins for OpenCOVER, e.g. for connecting to Vistle
        - `vistle/cover/RhrClient`: OpenCOVER remote hybrid rendering client plugin
