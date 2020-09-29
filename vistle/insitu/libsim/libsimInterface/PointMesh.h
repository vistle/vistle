// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_POINTMESH_H
#define SIMV2_POINTMESH_H
#include "export.h"
#include "VisItDataTypes.h"

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C" {
#endif

V_LIBSIMXPORT int simv2_PointMesh_alloc(visit_handle *);
V_LIBSIMXPORT int simv2_PointMesh_free(visit_handle);

V_LIBSIMXPORT int simv2_PointMesh_setCoordsXY(visit_handle obj, visit_handle x, visit_handle y);
V_LIBSIMXPORT int simv2_PointMesh_setCoordsXYZ(visit_handle obj, visit_handle x, visit_handle y, visit_handle z);
V_LIBSIMXPORT int simv2_PointMesh_setCoords(visit_handle obj, visit_handle xyz);

V_LIBSIMXPORT int simv2_PointMesh_getCoords(visit_handle h, int *ndims, int *coordMode, visit_handle *x,
                                            visit_handle *y, visit_handle *z, visit_handle *coords);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2
V_LIBSIMXPORT int simv2_PointMesh_check(visit_handle h);

#endif
