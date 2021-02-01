/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module for Tsunami data         	                   **
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

#include <vistle/module/reader.h>

#ifdef OLD_NETCDFCXX
#include <netcdfcpp>
#else
#include <ncFile.h>
#include <ncVar.h>
#include <ncDim.h>

using namespace netCDF;
#endif

#define NUMPARAMS 6

class ReadTsunami: public vistle::Reader {
public:
    //default constructor
    ReadTsunami(const std::string &name, int moduleID, mpi::communicator comm);
    virtual ~ReadTsunami() override;

private:
    //Vistle functions
    /* bool prepareRead() override; */
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;

    //Own functions
    bool openNcFile();
    void block(Token &token, vistle::Index bx, vistle::Index by, vistle::Index bz, vistle::Index b,
               vistle::Index time) const;

    //Parameter
    vistle::StringParameter *m_filedir;
    /* vistle::StringParameter *m_grid_choice_x, *m_grid_choice_y, *m_grid_choice_z; */
    vistle::StringParameter *m_variables[NUMPARAMS];
    vistle::FloatParameter *m_verticalScale;
    vistle::IntParameter *m_step;
    vistle::IntParameter *m_blocks[3];
    vistle::IntParameter *m_ghostLayerWidth;
    vistle::IntParameter *m_size[3];

    //Ports
    vistle::Port *m_grid_out = nullptr;
    vistle::Port *m_dataOut[NUMPARAMS];
    vistle::Port *m_surface_out;
    vistle::Port *m_seaSurface_out;
    vistle::Port *m_waterSurface_out;
    vistle::Port *m_maxHeight;

    //netCDF file to be read
    NcFile *ncDataFile;
};

#endif
