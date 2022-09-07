/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see LICENSE.txt.

 * License: LGPL 2+ */

#ifndef _ELEMENT_H_
#define _ELEMENT_H_
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
#endif // _ELEMENT_H_
