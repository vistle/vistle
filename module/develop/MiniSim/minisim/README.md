# Oscillators

The code computes a sum of damped, decaying, or periodic oscillators, convolved
with (unnormalized) Gaussians, on a grid. The analysis computes the sum of all
voxel-wise autocorrelations for all window sizes up to the specified input (via
`-w` option). It also finds the voxels with the `k` strongest autocorrelations
(for every shift).

The oscillators' locations and parameters are specified in an input file. See
inputs folder for examples. The `generate_input.py` script can generate a set
of randomly initialized oscillators.

The simulation code is in `oscillator.cpp`.
The analysis interface is specified in `analysis.h`.
The actual analysis code is in `analysis.cpp`.

To run:
```bash
mpirun -n ... ./oscillator sample.osc
```

Usage (**ENABLE_SENSEI=ON**):
```bash
./bin/oscillator [OPTIONS] OSCILLATORS.txt
Options:
    -b, --blocks INT      number of blocks to use [default: 1]
    -s, --shape POINT     domain shape [default: 64 64 64]
    -t, --dt FLOAT        time step [default: 0.01]
    -f, --config STRING   SENSEI analysis configuration xml (required)
    --t-end FLOAT         end time [default: 10]
    --sync                synchronize after each time step
   -h, --help             show help
```

Usage (**ENABLE_SENSEI=OFF**):
```bash
./bin/oscillator [OPTIONS] OSCILLATORS.txt
Options:
    -b, --blocks INT             number of blocks to use [default: 1]
    -s, --shape POINT            domain shape [default: 64 64 64]
    -t, --dt FLOAT               time step [default: 0.01]
    -w, --window UNSIGNED LONG   analysis window [default: 10]
    -k, --k-max UNSIGNED LONG    number of strongest autocorrelations to report [default: 3]
    --t-end FLOAT                end time [default: 10]
    --sync                       synchronize after each time step
    -h, --help                   show help
```
