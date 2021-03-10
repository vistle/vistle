/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module for ChEESE Seismic HDF5/XDMF-files  	       **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:    Marko Djuric                                                **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Date:  04.03.2021                                                      **
\**************************************************************************/

#include "ReadSeisSol.h"
#include <H5Exception.h>
#include <H5File.h>
#include <boost/mpi/communicator.hpp>

using namespace vistle;
using namespace H5;

MODULE_MAIN(ReadSeisSol)

ReadSeisSol::ReadSeisSol(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader("Read ChEESE Seismic Data files (xmdf/hdf5)", name, moduleID, comm)
{}

ReadSeisSol::~ReadSeisSol()
{}

bool ReadSeisSol::read(Token &token, int timestep, int block)
{
    return true;
}

/* bool examine(const vistle::Parameter *param) */
/* { */
/*     return true; */
/* } */
