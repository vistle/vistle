// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef VISIT_DATA_INTERFACE_RUNTIME_P_H
#define VISIT_DATA_INTERFACE_RUNTIME_P_H
#include <stdlib.h>
#include <cstddef>
#include "VisItDataTypes.h"

/* This file contains prototypes of functions that are used internally
   in the data interface. These functions are never exposed beyond the
   runtime.
 */

class VisIt_ObjectBase {
public:
    VisIt_ObjectBase(int t);
    virtual ~VisIt_ObjectBase();

    int objectType() const;

private:
    int object_type;
};

VisIt_ObjectBase *VisItGetPointer(visit_handle h);
void VisItFreePointer(visit_handle h);
visit_handle VisItStorePointer(VisIt_ObjectBase *ptr);

void VisItError(const char *msg);

#endif
