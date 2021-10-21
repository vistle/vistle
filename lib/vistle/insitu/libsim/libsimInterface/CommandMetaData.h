// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_COMMANDMETADATA_H
#define SIMV2_COMMANDMETADATA_H
#include "export.h"
#include "VisItDataTypes.h"

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C" {
#endif

V_LIBSIMXPORT int simv2_CommandMetaData_alloc(visit_handle *obj);
V_LIBSIMXPORT int simv2_CommandMetaData_free(visit_handle obj);
V_LIBSIMXPORT int simv2_CommandMetaData_setName(visit_handle h, const char *);
V_LIBSIMXPORT int simv2_CommandMetaData_getName(visit_handle h, char **);
V_LIBSIMXPORT int simv2_CommandMetaData_setEnabled(visit_handle h, int);
V_LIBSIMXPORT int simv2_CommandMetaData_getEnabled(visit_handle h, int *);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2
V_LIBSIMXPORT int simv2_CommandMetaData_check(visit_handle);

#endif
