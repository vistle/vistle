/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module Elmer data format        	                   **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:    Marko Djuric <hpcmdjur@hlrs.de>                             **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Date:  20.03.2023 Version 1                                            **
\**************************************************************************/

#ifndef _READSOUNDPLAN_H
#define _READSOUNDPLAN_H

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
