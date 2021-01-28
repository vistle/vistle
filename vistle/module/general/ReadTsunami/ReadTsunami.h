
/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module for WRFChem data         	                   **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:    Marko Djuric                                                **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Date:  25.01.2021                                                      **
\**************************************************************************/
#ifndef _READTSUNAMI_H
#define _READTSUNAMI_H

#include <boost/mpi/communicator.hpp>
#include <vistle/module/reader.h>
#include <vistle/core/parameter.h>
#include <vistle/core/port.h>
#include <vistle/core/structuredgrid.h>

#ifdef OLD_NETCDFCXX
#include <netcdfcpp>
#else
#include <ncFile.h>
#include <ncVar.h>
#include <ncDim.h>
using namespace netCDF;
#endif

#define NUMPARAMS 6

using namespace vistle;

class ReadTsunami: public vistle::Reader {
public:
    //default constructor
    ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm);
    
    //destructor
    virtual ~ReadTsunami() override;

private:
    //Vistle functions
    bool prepareRead() override;
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;

    //Own functions
    bool openNcFile();

    //Parameter
    StringParameter *m_filedir;
    StringParameter *m_grid_choice_x, *m_grid_choice_y, *m_grid_choice_z;
    StringParameter *m_variables[NUMPARAMS];
    FloatParameter *m_verticalScale;
    IntParameter *m_step;

    //Ports
    Port *m_grid_out = nullptr;
    Port *m_dataOut[NUMPARAMS];
    Port *m_surface_out;
    Port *m_seaSurface_out;
    Port *m_maxHeight;
    Port *m_waterSurface_out;

    //netCDF file to be read
    NcFile *ncDataFile;
};

#endif
