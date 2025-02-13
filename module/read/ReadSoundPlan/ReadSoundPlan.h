#ifndef VISTLE_READSOUNDPLAN_READSOUNDPLAN_H
#define VISTLE_READSOUNDPLAN_READSOUNDPLAN_H

/**************************************************************************\
 **                                                                        **
 ** Description: Read module for SoundPLAN data format (Port from COVISE)  **
 **                                                                        **
 ** Author:    Marko Djuric <hpcmdjur@hlrs.de>                             **
 **                                                                        **
 ** Date:  20.03.2023 Version 1                                            **
\**************************************************************************/

#include <vistle/module/reader.h>
#include "vistle/core/parameter.h"

class ReadSoundPlan: public vistle::Reader {
public:
    //constructor
    ReadSoundPlan(const std::string &name, int moduleID, mpi::communicator comm);

private:
    //Vistle functions
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;

    //Parameter
    vistle::Port *m_meshOutPort;
    vistle::Port *m_daySoundOutPort;
    vistle::Port *m_nightSoundOutPort;

    vistle::StringParameter *m_file;
};
#endif
