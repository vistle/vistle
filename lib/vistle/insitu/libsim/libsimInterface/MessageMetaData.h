// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_MESSAGEMETADATA_H
#define SIMV2_MESSAGEMETADATA_H
#include "export.h"
#include "VisItDataTypes.h"

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C" {
#endif

V_LIBSIMXPORT int simv2_MessageMetaData_alloc(visit_handle *obj);
V_LIBSIMXPORT int simv2_MessageMetaData_free(visit_handle obj);
V_LIBSIMXPORT int simv2_MessageMetaData_setName(visit_handle h, const char *);
V_LIBSIMXPORT int simv2_MessageMetaData_getName(visit_handle h, char **);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2
V_LIBSIMXPORT int simv2_MessageMetaData_check(visit_handle);

#endif
