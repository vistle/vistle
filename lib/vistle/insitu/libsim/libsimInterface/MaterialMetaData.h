// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_MATERIALMETADATA_H
#define SIMV2_MATERIALMETADATA_H
#include "export.h"
#include "VisItDataTypes.h"

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C" {
#endif

V_LIBSIMXPORT int simv2_MaterialMetaData_alloc(visit_handle *obj);
V_LIBSIMXPORT int simv2_MaterialMetaData_free(visit_handle obj);
V_LIBSIMXPORT int simv2_MaterialMetaData_setName(visit_handle h, const char *);
V_LIBSIMXPORT int simv2_MaterialMetaData_getName(visit_handle h, char **);
V_LIBSIMXPORT int simv2_MaterialMetaData_setMeshName(visit_handle h, const char *);
V_LIBSIMXPORT int simv2_MaterialMetaData_getMeshName(visit_handle h, char **);
V_LIBSIMXPORT int simv2_MaterialMetaData_addMaterialName(visit_handle h, const char *);
V_LIBSIMXPORT int simv2_MaterialMetaData_getNumMaterialNames(visit_handle h, int *);
V_LIBSIMXPORT int simv2_MaterialMetaData_getMaterialName(visit_handle h, int, char **);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2
V_LIBSIMXPORT int simv2_MaterialMetaData_check(visit_handle);

#endif
