/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see LICENSE.txt.

 * License: LGPL 2+ */

#ifndef _READDYNA3D_H
#define _READDYNA3D_H
/**************************************************************************\ 
 **                                                           (C)1995 RUS  **
 **                                                                        **
 ** Description: Read module for Dyna3D data         	                  **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:                                                                **
 **                                                                        **
 **                             Uwe Woessner                               **
 **                Computer Center University of Stuttgart                 **
 **                            Allmandring 30                              **
 **                            70550 Stuttgart                             **
 **                                                                        **
 ** Date:  17.03.95  V1.0                                                  **
 ** Revision R. Beller 08.97 & 02.99                                       **
\**************************************************************************/
/////////////////////////////////////////////////////////
//                I M P O R T A N T
/////////////////////////////////////////////////////////
// NumState keeps the number of time steps to be read
// (apart from the first one).
// MaxState is kept equal to MinState+NumState.
// Now State is also a redundant variable which is kept
// equal to MaxState.
// These redundancies have been introduced in order to facilitate
// that the authors of this code (or perhaps
// someone else who has the courage to accept
// the challenge) may easily recognise its original state and
// eventually improve its functionality and correct
// its original flaws in spite of the
// modifications that have been introduced to make
// the parameter interface (of parameter p_Step,
// the former parameter p_State) simpler.
// These last changes have not contributed perhaps
// to make the code more clear, but also not
// darker as it already was.
//
// The main problem now is how to determine the first time step
// to be read. It works for the first (if MinState==1), but
// the behaviour is not very satisfactory in other cases.
// This depends on how many time steps are contained per file.
// A second improvement would be to read time steps with
// a frequency greater than 1.
/////////////////////////////////////////////////////////

//#include <appl/ApplInterface.h>

#include <vistle/util/coRestraint.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>

#include <vistle/module/reader.h>

#include "Element.h"
#include "Dyna3DReader.h"

class ReadDyna3D: public vistle::Reader {
public:
private:
    bool prepareRead() override;
    bool read(Token &token, int timestep = -1, int block = -1) override;
    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;

    Dyna3DReaderBase *newReader();

    // Ports
    vistle::Port *p_grid = nullptr;
    vistle::Port *p_data_1 = nullptr;
    vistle::Port *p_data_2 = nullptr;

    // Parameters
    vistle::StringParameter *p_data_path;
    vistle::IntParameter *p_nodalDataType;
    vistle::IntParameter *p_elementDataType;
    vistle::IntParameter *p_component;
    vistle::StringParameter *p_Selection;
    // coIntSliderParam *p_State;
    //vistle::IntVectorParameter *p_Step;
    vistle::IntParameter *p_format;
    vistle::IntParameter *p_only_geometry;
    vistle::IntParameter *p_byteswap;

public:
    ReadDyna3D(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadDyna3D() override;

    std::unique_ptr<Dyna3DReaderBase> dyna3dReader;
};
#endif // _READDYNA3D_H
