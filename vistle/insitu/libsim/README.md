VisIt's LibSim interface
=====================================================
This builds the parallel runtime library (libsimV2runntime_par) a LibSim instrumented simulation dynamically links against.
To use with serial simulations disable LIBSIM_PARALLEL in CMake options.

To link a simulation to Vislte instead of VisIt remove VisIt specific entries from LD_LIBRARY_PATH and add Vistle's lib directory instead.

To connect Vistle to the simulation there are two different strategies depending on the build mode of Vistle.

If Vistle is build as multi-process shared memory program:

	start Vistle and start the LibSim module. 
	Open the simulations .sim2 file with this module and it should connect to the simulation.
	
If Vistle is build as single-process program:

	set environmentvariables:
		VISTLE_ROOT=path/to/vistle/build-dir (in the sim shell)
		Vistle_KEY=1234 (same number in the hub and sim shell)
	start sim
	start Vistle with the command-line-argument --libsim {path/to/.sim2/file} 
	Vistle starts in the simulation's process and connects to the hub
	start the LibSim module to view and control the simulation
	
connectLibsim
-------------
A small library that manages the first connection to a LibSim instrumented simulation that initiates the linking of the sim to the libsimV2runntime library.

engineInterface
---------------
In single process mode this provides an interface for the LibSimController module to get access to the simulation's control socket.  

libsimInterface
---------------
Contains the libsim files that represent the runtime interface to the libsim static library
