// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_SPECIESMETADATA_H
#define SIMV2_SPECIESMETADATA_H
#include "export.h"
#include "VisItDataTypes.h"

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C" {
#endif

V_LIBSIMXPORT int simv2_SpeciesMetaData_alloc(visit_handle *obj);
V_LIBSIMXPORT int simv2_SpeciesMetaData_free(visit_handle obj);
V_LIBSIMXPORT int simv2_SpeciesMetaData_setName(visit_handle h, const char *);
V_LIBSIMXPORT int simv2_SpeciesMetaData_getName(visit_handle h, char **);
V_LIBSIMXPORT int simv2_SpeciesMetaData_setMeshName(visit_handle h, const char *);
V_LIBSIMXPORT int simv2_SpeciesMetaData_getMeshName(visit_handle h, char **);
V_LIBSIMXPORT int simv2_SpeciesMetaData_setMaterialName(visit_handle h, const char *);
V_LIBSIMXPORT int simv2_SpeciesMetaData_getMaterialName(visit_handle h, char **);
V_LIBSIMXPORT int simv2_SpeciesMetaData_addSpeciesName(visit_handle h, visit_handle);
V_LIBSIMXPORT int simv2_SpeciesMetaData_getNumSpeciesName(visit_handle h, int *);
V_LIBSIMXPORT int simv2_SpeciesMetaData_getSpeciesName(visit_handle h, int, visit_handle *);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2
V_LIBSIMXPORT int simv2_SpeciesMetaData_check(visit_handle);

#endif
