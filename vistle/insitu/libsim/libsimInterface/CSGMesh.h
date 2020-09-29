// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_CSGMESH_H
#define SIMV2_CSGMESH_H
#include "export.h"
#include "VisItDataTypes.h"

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C" {
#endif

V_LIBSIMXPORT int simv2_CSGMesh_alloc(visit_handle *);
V_LIBSIMXPORT int simv2_CSGMesh_free(visit_handle);

V_LIBSIMXPORT int simv2_CSGMesh_setRegions(visit_handle h, visit_handle typeflags, visit_handle left,
                                           visit_handle right);

V_LIBSIMXPORT int simv2_CSGMesh_setZonelist(visit_handle h, visit_handle zl);
V_LIBSIMXPORT int simv2_CSGMesh_setBoundaryTypes(visit_handle h, visit_handle cshtypes);
V_LIBSIMXPORT int simv2_CSGMesh_setBoundaryCoeffs(visit_handle h, visit_handle coeffs);
V_LIBSIMXPORT int simv2_CSGMesh_setExtents(visit_handle h, double min[3], double max[3]);

V_LIBSIMXPORT int simv2_CSGMesh_getRegions(visit_handle h, visit_handle *typeflags, visit_handle *left,
                                           visit_handle *right);
V_LIBSIMXPORT int simv2_CSGMesh_getZonelist(visit_handle h, visit_handle *zl);
V_LIBSIMXPORT int simv2_CSGMesh_getBoundaryTypes(visit_handle h, visit_handle *cshtypes);
V_LIBSIMXPORT int simv2_CSGMesh_getBoundaryCoeffs(visit_handle h, visit_handle *coeffs);
V_LIBSIMXPORT int simv2_CSGMesh_getExtents(visit_handle h, double min[3], double max[3]);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2
V_LIBSIMXPORT int simv2_CSGMesh_check(visit_handle h);

#endif
