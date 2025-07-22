# To make non-mpi versions just use make:
#     make
#
# To make the mpi version of the files run with:
#     make mpiprefix=mpi
#
EXECUTABLE=flushhexa27partitioned
CPPFILES=$(EXECUTABLE).cpp
F90FILES=$(EXECUTABLE)f90.F90

ifdef mpiprefix
  ifndef DISABLE_MPICPP
    TARGETS=mpicppbuild
  endif
# FORTRAN example not yet implemented
#  ifndef DISABLE_MPIFORTRAN
#    TARGETS += $(mpiprefix)f90build
#  endif
else
  TARGETS=cppbuild
# FORTRAN example not yet implemented
#  ifndef DISABLE_FORTRAN
#    TARGETS += f90build
#  endif
endif


ifdef TARGETS
  include ../base.make
endif
