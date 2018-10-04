#! /bin/bash

#PBS -N Vistle
#PBS -l walltime=00:20:00             
#xPBS -l nodes=1:ppn=2
#xPBS -L tasks=2:lprocs=24:allowthreads
#PBS -l nodes=4:ppn=2

export OMP_NUM_THREADS=12
  
# Change to the directory that the job was submitted from
cd $PBS_O_WORKDIR

# enable core dumps
export ATP_ENABLED=1
ulimit -c unlimited

# load modules
module swap PrgEnv-cray PrgEnv-gnu
module load tools/vtk/8.1.0
#module load numlib/intel/tbb/2017.4
module load tools/boost/1.66.0

VDIR="$HOME/vistle/build-xc40"
export VISTLE_LAUNCH=aprun
#export VISTLE_LAUNCH=manual

#module load forge
#export VISTLE_LAUNCH=ddt-aprun
export MPICH_MAX_THREAD_SAFETY=multiple
export MPICH_VERSION_DISPLAY=1

export PATH="$VDIR/bin:$PATH"
export LD_LIBRARY_PATH="$VDIR/lib:$HOME/hazelhen/lib:$LD_LIBRARY_PATH:$PYTHON_DIR/lib"

#vistle -b -c visard.hlrs.de > vistle.out 2>&1
vistle -b -c 141.58.8.1
