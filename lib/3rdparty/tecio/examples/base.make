# Set to appropriate C++ compilers
           CPP=g++
        MPICPP=mpic++
      # add flag -O3 for optimized builds, or -g for debug builds
      CPPFLAGS=-std=c++14

# Set to appropriate Fortran compilers
            FC=gfortran
         MPIFC=mpif90
        FFLAGS=-fcray-pointer

#
# If you are on an old linux platform you "may" be required to use the c++ library
# employed when building the tecio library.  If you receive link errors when building
# try enabling this setting.
#
        USE_TECPLOT_PROVIDED_CPP_LIB=NO

#
# Determine what context we are in:
#     a Tecio source distribution
#     a Tecplot installation
#     Use a Tecplot installation but example source is not in the tecplot installation
#
#
# NOTE: Currently cannot run straight out of a tecplot sandbox because not all of
#       the include files are easy to track down.


ISTECIOSRC := $(shell test -d ../../teciosrc; echo $$?)

$(info isteciosrc = $(ISTECIOSRC))

ifeq ($(ISTECIOSRC),0)
    TECIOMPILIB=../../teciompisrc/libteciompi.a
    TECIOLIB=../../teciosrc/libtecio.a
    ifeq ($(shell uname -s),Darwin)
        # LLVM on the Mac links against libc++
        EXTRALIBS=-lc++
    else
        EXTRALIBS=-lstdc++
    endif
    EXTRAINCLUDES=-I../../teciosrc
else # Example source is in a tecplot installation
    #
    # It may be the case that we actually want to use a different tecplot home
    # from which to run the executable.  This allows the example source to be
    # put in a temporary location that can be written to.  If this is desired
    # then define TECPLOT_HOME_DIR to be the tecplot home directory.
    #
    ifndef TECPLOT_HOME_DIR
        TECPLOT_HOME_DIR=../../../..
    endif

    ifeq ($(shell uname -s),Darwin)
        # LLVM on the Mac links against libc++
        EXTRALIBS=-lc++
        TECIOLIB=$(TECPLOT_HOME_DIR)/Frameworks/libtecio.dylib
        TECIOMPILIB=$(TECPLOT_HOME_DIR)/Frameworks/libteciompi.dylib
    else
        EXTRALIBS=-lstdc++
        TECIOMPILIB=$(TECPLOT_HOME_DIR)/bin/libteciompi.so
        TECIOLIB=$(TECPLOT_HOME_DIR)/bin/libtecio.so
    endif
    FOUND_INSTALLED_LIBSTDCXX_S := $(shell test -f $(TECPLOT_HOME_DIR)/bin/sys/libstdc++.so.6 && echo found || echo missing)


    # See note above for USE_TECPLOT_PROVIDED_CPP_LIB
    ifeq ($(FOUND_INSTALLED_LIBSTDCXX_S),found)
        ifeq ($(USE_TECPLOT_PROVIDED_CPP_LIB),YES)
            EXTRALIBS=$(TECPLOT_HOME_DIR)/bin/sys/libstdc++.so.6
        endif
    endif

    EXTRAINCLUDES=-I$(TECPLOT_HOME_DIR)/include
endif



#
# (If not needed reset to empty in actual Makefile)
# link libraries
      LINKLIBS=-lpthread -lm

   PATHTOEXECUTABLE=.

all: $(TARGETS)

clean:
	rm -f $(PATHTOEXECUTABLE)/$(EXECUTABLE)        > /dev/null 2>&1
	rm -f $(PATHTOEXECUTABLE)/$(EXECUTABLE)f      > /dev/null 2>&1
	rm -f $(PATHTOEXECUTABLE)/$(EXECUTABLE)mpi    > /dev/null 2>&1
	rm -f $(PATHTOEXECUTABLE)/$(EXECUTABLE)f90    > /dev/null 2>&1
	rm -f $(PATHTOEXECUTABLE)/$(EXECUTABLE)mpif90 > /dev/null 2>&1

cppbuild:
	$(CPP) $(CPPFLAGS) $(CPPFILES) $(EXTRAINCLUDES) $(TECIOLIB) $(EXTRALIBS) $(LINKLIBS) -o $(PATHTOEXECUTABLE)/$(EXECUTABLE)

mpicppbuild:
	$(MPICPP) $(CPPFLAGS) -DTECIOMPI $(CPPFILES) $(EXTRAINCLUDES) $(TECIOMPILIB) $(EXTRALIBS) $(LINKLIBS) -o $(PATHTOEXECUTABLE)/$(EXECUTABLE)mpi

fbuild:
	$(FC) $(FFILES) $(FFLAGS) $(EXTRAINCLUDES) $(TECIOLIB) $(EXTRALIBS) $(LINKLIBS) -o $(PATHTOEXECUTABLE)/$(EXECUTABLE)f

f90build:
	$(FC) $(F90FILES) $(FFLAGS) $(EXTRAINCLUDES) $(TECIOLIB) $(EXTRALIBS) $(LINKLIBS) -o $(PATHTOEXECUTABLE)/$(EXECUTABLE)f90

mpif90build:
	$(MPIFC) -DTECIOMPI $(F90FILES) $(FFLAGS) $(EXTRAINCLUDES) $(TECIOMPILIB) $(EXTRALIBS) $(LINKLIBS) -o $(PATHTOEXECUTABLE)/$(EXECUTABLE)f90mpi
