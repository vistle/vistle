VisIt's LibSim interface
=====================================================
This builds the paralell runtime library (libsimV2runntime_par) a LibSim instrumented simulation dynamically links against.
To use with serial simulations disable LIBSIM_PARALELL in CMake options.

To link a simulation to Vislte instead of VisIt remove VisIt specific entries from LD_LIBRARY_PATH and add Vistle's lib directory instead.

To connect Vistle to the simulation there are two different strategies depending on the build mode of Vistle.

If Vistle is build as multi-process shared memory programm:

	start Vistle and start the LibSim module. 
	Open the simulations .sim2 file with this module and it should connect to the simulation.
	
If Vistle is build as single-process programm:

	start Vistle with the command-line-argument --libsim {path/to/.sim2/file} 
	Vistle starts in the simulation's process and connects to the hub
	start the LibSim module to viwe and control the simulation
	
EstablishLibSimConnection
--------------------------
A small library that manages the first connection to a LibSim instrumented simulation that initiates the linking to the libsimV2runntime library