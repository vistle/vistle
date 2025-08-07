EXECUTABLE = simtest
CPPFILES   = $(EXECUTABLE).cpp
FFILES     = $(EXECUTABLE).f
F90FILES   = $(EXECUTABLE).f90

TARGETS=cppbuild 
ifndef DISABLE_FORTRAN
TARGETS += fbuild f90build
endif


include ../base.make
