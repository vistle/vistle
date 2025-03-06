[headline]:<>

## Purpose

Retrieve simulation data during the runtime of LibSim instrumented simulations.

## Data Preparation
The LibSim instrumented simulation must initialize MPI with MPI_Init_thread and thread support MPI_THREAD_MULTIPLE.

## Ports

[moduleHtml]:<>

Ports are dynamically created depending on the data the simulation provides. Data for unconnected ports
is not requested from LibSim.

[parameters]:<>

Depending on the connected ports and the frequency parameter the module requests data from the simulation.
Once the simulation generates the requested data the module is automatically executed.

### Simulation Specific Commands
Connected simulations might provide additional commands that are (for now) represented by parameters. Changing one of these parameters triggers the displayed action in the simulation, while the actual value of bolean parameters is meaningless the simulation can also specify commands that take a string as argument. These commands are represented as string input parameters
and send their string value to the simulation on change.

## Usage Examples

### Mandelbrot example
-Download the [Mandelbrot simulation](https://www.visitusers.org/index.php?title=VisIt-tutorial-in-situ#Resources) and edit the Makefile as described. Additionally set "CXX=mpic++" and add -DPARALLEL to CXXFLAGS  
-Open mandelbrot.C and change:
```cpp
    /* Initialize MPI */
    MPI_Init(&argc, &argv);
``` 
to
```cpp
    /* Initialize MPI */
    int provided = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
``` 
#### For multi process Vistle:
-build and run the simulation. 
```bash
make
export LD_LIBRARY_PATH=~/src/vistle/build/lib"
mpirun -np 2 ./mandelbrot
```

-start vistle in a new terminal
```bash
export MPISIZE=2
vistle
``` 
-start the LibSimController module and set the path parameter to the LibSim file.   
The default location for these files is:  
$HOME/.visit/simulations/*hash*.mandelbrot_par.sim2.  

#### For single process Vistle:
-build and run the simulation. 
```bash
make
export VISTLE_ROOT=~/src/vistle/build/
export LD_LIBRARY_PATH=$VISTLE_ROOT/lib"
#to use COVER
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/src/covise/archsuffix/lib"
export COVISEDIR=$HOME/src/covise
export COVISE_PATH=$VISTLE_ROOT:$COVISEDIR


mpirun -np 2 ./mandelbrot
```

-start vistle hub in a new terminal with the LibSim start option and the path to the sim2 file
```bash
export MPISIZE=2
vistle -l $HOME/.visit/simulations/hash.mandelbrot_par.sim2
``` 
-start the LibSimController module
#### For both versions:

The module should now display two output ports: AMR mesh and mandelbrot.  
-enable VTK variables for the LibSimController module  
-connect the mandelbrot port with the following modues as shown in the picture:

![](mandelbrot_map.png)

-enable quad for Domainsurface  
-pres step in the LibSimController module to execute one timestep or type *step* in the commandline of the simulation
-in COVER *view all* and rotate the image. Use *Navigation/Center view* to allign the image. The rusult should look like:

![](mandelbrot.png)

## Related Modules
SenseiController

## Acknowledgements
Developed through funding from the EXCELLERAT centre of excellence.
