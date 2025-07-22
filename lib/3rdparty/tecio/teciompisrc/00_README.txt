Building the teciompi library.

Licensing
-------------
See tecio_license_agreement.txt for license information.

Prerequisites:
-------------
Compiler: Support of the C++14 standard.

Boost:
  Regardless of which build option you choose below you must have access to boost
  (minimum version required: 1.69.0), or have a Boost source distribution
  available. 

  Tecio requires only the Boost header files--no libraries--so it is possible to
  build tecio using a Boost source distribution only, without installing it. In
  either case you will need to know the path to the root of the boost include
  files.


Building the library:
---------------------
I.  On Linux/Mac OS X
    A. Using cmake

        If you have cmake 3.0 or newer on you computer you may do the following

        Run cmake:
           cmake .

        At this point if you get a warning about not finding boost then do
        the following:
            1.  Edit CMakeLists.txt file and set the path to BOOST_INCLUDE_DIR 
                to the directory above the boost folder containing the boost 
                header files.   For example, if you extracted a Boost source
                archive to "/home/me/boost_1_71_0", then set BOOST_INCLUDE_DIR
                to "/home/me/boost_1_71_0".
            2.  Rerun cmake

                 cmake .
        
        Run make
           make

           (Optionally add -j n to run n concurrent compiles.  For example, if 
           you have 64 cores then use -j 64)

    B. Using make

        If you do not have cmake then you can use the supplied Makefile.nix
        using:

            make BOOST_ROOT=iii -f Makefile.nix

        where iii is the path to the boost root directory

        (Optionally add -j n to run n concurrent compiles.  For example, if 
        you have 64 cores then use -j 64)

     If you find that you have to modify Makefile.nix to work on your system,
     please let us know about your changes so we can provide a customized
     makefile with our distribution.

II. On Windows

    CMake 3.0 or later is required. You can download and install this from
    www.cmake.org. The below instructions assume you're using some version of
    Visual Studio, but CMake also supports a number of other build environments
    on Windows. If you use some other build environment, you'll probably need
    to modify some compiler flags before you click Generate below; refer
    to your compiler's documentation for these.

    A. Run CMake
        1. Launch the CMake GUI (Start/All Programs/CMake/CMake(cmake-gui)).
        2. Set "Where is the source code" to the extracted teciompisrc directory,
           such as D:/teciompisrc
        3. Set "Where to build the binaries" to any desired directory. CMake
           will create this directory for you if necessary.
        4. Click Configure
            a. Click Yes if CMake asks to create the directory.
            b. Select the desired version of Visual Studio and word length
               (only 64-bit is supported). For example, for 64-bit builds with
               Visual Studio 2012, select "Visual Studio 11 Win64"
            c. Click Finish
        5. Ensure the "Advanced" toggle is set, and find variable
           Boost_INCLUDE_DIR. If Boost was not found--if CMake shows
           Boost_INCLUDE_DIR set to Boost_INCLUDE_DIR_NOTFOUND--then set
           Boost_INCLUDE_DIR to point to the boost install folder, such as
           C:/local/boost_1_69_0 (see the note on Boost below).
        6. Click Configure again.
        7. Click Generate. You should see no errors, and "Generating done" in
           CMake's text output window. CMake has now created solution and
           project files for your version of Visual Studio.
        8. Quit CMake

    B. Run Visual Studio
        1. Launch the version of Visual Studio you selected in step 4b above.
        2. Select File/Open/Project/Solution...
        3. Navigate to the binary folder you selected in step 3 above.
        4. Select teciompi.sln and click Open.
        5. If necessary, display the Standard toolbar (View/Toolbars/Standard).
        6. Select Debug or Release in the Standard toolbar.
        7. Select Build/Build Solution. You should see no compiler warnings or
           errors.
        8. teciompi.lib is now available in the Debug or Release subdirectory of
           the binary build folder you selected above.


Using the library:
-----------------
Please refer to the Data Format Guide (360-data-format.pdf) and Tecio
example programs for how to use this library. If you wish to copy only the
required files to a different folder for later use, please copy the following
files:

libteciompi.a (teciompi.lib on Windows)
StandardIntegralTypes.h
TECIO.h
tecio_Exports.h
tecio.inc
tecio.for
tecio.f90
tecio_license_agreement.txt


A Note on Boost: Tecio requires only the Boost header files--no libraries--so
it is possible to build teciompi using a Boost source distribution only, without
installing it. In this case, set Boost_INCLUDE_DIR to point to the root of
the source distribution, such as D:/boost_1_69_0, then click Configure, then
Generate in the CMake GUI (or for Linux, edit the Boost_INCLUDE_DIR setting in
CMakeLists.txt and reissue "cmake ." from the command line to recreate Makefile).
