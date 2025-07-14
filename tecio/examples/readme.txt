Subfolders below this folder contain example applications that use the tecio library.

Building the example applications:

Linux:
------
  cd to the example folder you would like to build.
  Type:
        make <cr>

  Notes:  
    1) Edit the Makefile for the example to change the targets you want to build.
       Not all examples are set up to build using the mpi tecio library.
    2) If you are building against the tecio (or teciompi) library built using the source code
       care package then see instructions in base.make located in this folder.

Windows:
--------
  Developer Studio solution files are provided to help you build the example 
  applications.

Building all examples
---------------------
  After building the TecIO and/or TecIO-MPI libraries, run file buildandtestall.py
  using a Python 3 interpreter. On Windows, you'll need to do this in a command window
  with PATH set to include compilers. Visual Studio installations provide Start Menu
  shortcuts to provide these, such as:
      Start Menu/Visual Studio 2022/x64 Native Tools Command Prompt for VS 2022

  buildandtestall.py produces text file output:
    examples-buildandtestsummary.txt
      A results summary of example building and testing. The result of each build and execution
      is marked with status "Passed," "Error," or, if an example wasn't built, the status is empty.
    examples-builddetail.txt
      The detailed output from building each example
    examples-executiondetail.txt
      The detailed output from running each non-MPI example
    examples-mpi-executiondetail.txt
      The detailed output from running each MPI example
