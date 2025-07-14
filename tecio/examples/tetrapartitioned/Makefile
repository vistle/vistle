# To make non-mpi versions just use make:
#     make
#
# To make the mpi version of the files run with:
#     make mpiprefix=mpi
#
EXECUTABLE=tetrapartitioned
CPPFILES=$(EXECUTABLE).cpp
F90FILES=$(EXECUTABLE)f90.F90

TARGETS=$(mpiprefix)cppbuild

ifdef mpiprefix
  ifndef DISABLE_MPICPP
    TARGETS=mpicppbuild
  endif
  ifndef DISABLE_MPIFORTRAN
    TARGETS += $(mpiprefix)f90build
  endif
else
  TARGETS=cppbuild
  ifndef DISABLE_FORTRAN
    TARGETS += f90build
  endif
endif


ifdef TARGETS
  include ../base.make
endif
