# To make non-mpi versions just use make:
#     make
#
# To make the mpi version of the files run with:
#     make mpiprefix=mpi
#
EXECUTABLE=flushpartitioned
CPPFILES=$(EXECUTABLE).cpp

ifdef mpiprefix
  ifndef DISABLE_MPICPP
    TARGETS=mpicppbuild
  endif
else
  TARGETS += cppbuild
endif


ifdef TARGETS
  include ../base.make
endif
