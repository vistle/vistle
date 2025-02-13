#ifndef VISTLE_READDYNA3D_ELEMENT_H
#define VISTLE_READDYNA3D_ELEMENT_H

/**************************************************************************\ 
 **                                                           (C)1999 RUS  **
 **                                                                        **
 ** Description:  Class-Declaration of object "Element"                    **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:                                                                **
 **                                                                        **
 **                            Reiner Beller                               **
 **                Computer Center University of Stuttgart                 **
 **                            Allmandring 30                              **
 **                            70550 Stuttgart                             **
 **                                                                        **
 ** Date:                                                                  **
\**************************************************************************/


#include <vistle/core/scalar.h>
#include <vistle/core/index.h>
#include <vistle/core/unstr.h>

struct Element {
    vistle::Byte coType = vistle::UnstructuredGrid::NONE; // COVISE element type
    vistle::Index coElem = vistle::InvalidIndex; // COVISE element number
    int matNo = -1; // material number (of LS-DYNA plot file)
    //int node[8];        // node numbers (of LS-DYNA plot file) determining the element
    bool visible = true; // element visible?
};
#endif
