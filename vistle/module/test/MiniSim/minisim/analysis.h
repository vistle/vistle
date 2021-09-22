#pragma once

#include <mpi.h>

void init_analysis(MPI_Comm world, size_t window, size_t n_local_blocks, int domain_shape_x, int domain_shape_y,
                   int domain_shape_z, int *gid, int *from_x, int *from_y, int *from_z, int *to_x, int *to_y,
                   int *to_z);

void analyze(int gid, float *data);

void analysis_round();

void analysis_final(size_t k_max, size_t nblocks);
