EXECUTABLE=rewriteszl
CPPFILES=$(EXECUTABLE).cpp
FFILES=$(EXECUTABLE)f.F
F90FILES=$(EXECUTABLE)f90.F90

TARGETS=cppbuild 
ifndef DISABLE_FORTRAN
TARGETS += fbuild f90build
endif

include ../base.make
