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
#ifndef _READSEISSOL_H
#define _READSEISSOL_H

#include <vistle/module/reader.h>
#include <vistle/core/parameter.h>

#include <hdf5.h>

class ReadSeisSol: public vistle::Reader {
public:
    //default constructor
    ReadSeisSol(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadSeisSol() override;

private:
    //Vistle functions
    bool read(Token &token, int timestep, int block) override;
    /* bool examine(const vistle::Parameter *param) override; */
};
#endif
