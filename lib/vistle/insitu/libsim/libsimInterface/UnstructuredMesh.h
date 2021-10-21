// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_UNSTRUCTUREDMESH_H
#define SIMV2_UNSTRUCTUREDMESH_H
#include "export.h"
#include "VisItDataTypes.h"

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C" {
#endif

V_LIBSIMXPORT int simv2_UnstructuredMesh_alloc(visit_handle *);
V_LIBSIMXPORT int simv2_UnstructuredMesh_free(visit_handle);

V_LIBSIMXPORT int simv2_UnstructuredMesh_setCoordsXY(visit_handle obj, visit_handle x, visit_handle y);
V_LIBSIMXPORT int simv2_UnstructuredMesh_setCoordsXYZ(visit_handle obj, visit_handle x, visit_handle y, visit_handle z);
V_LIBSIMXPORT int simv2_UnstructuredMesh_setCoords(visit_handle obj, visit_handle xyz);
V_LIBSIMXPORT int simv2_UnstructuredMesh_setConnectivity(visit_handle obj, int nzones, visit_handle conn);
V_LIBSIMXPORT int simv2_UnstructuredMesh_setRealIndices(visit_handle obj, int, int);
V_LIBSIMXPORT int simv2_UnstructuredMesh_setGhostCells(visit_handle h, visit_handle gz);
V_LIBSIMXPORT int simv2_UnstructuredMesh_setGhostNodes(visit_handle h, visit_handle gn);
V_LIBSIMXPORT int simv2_UnstructuredMesh_setGlobalCellIds(visit_handle obj, visit_handle glz);
V_LIBSIMXPORT int simv2_UnstructuredMesh_setGlobalNodeIds(visit_handle obj, visit_handle gln);

V_LIBSIMXPORT int simv2_UnstructuredMesh_getCoords(visit_handle h, int *ndims, int *coordMode, visit_handle *x,
                                                   visit_handle *y, visit_handle *z, visit_handle *coords);
V_LIBSIMXPORT int simv2_UnstructuredMesh_getConnectivity(visit_handle h, int *nzones, visit_handle *conn);
V_LIBSIMXPORT int simv2_UnstructuredMesh_getRealIndices(visit_handle obj, int *, int *);
V_LIBSIMXPORT int simv2_UnstructuredMesh_getGhostCells(visit_handle h, visit_handle *gz);
V_LIBSIMXPORT int simv2_UnstructuredMesh_getGhostNodes(visit_handle h, visit_handle *gn);
V_LIBSIMXPORT int simv2_UnstructuredMesh_getGlobalCellIds(visit_handle obj, visit_handle *glz);
V_LIBSIMXPORT int simv2_UnstructuredMesh_getGlobalNodeIds(visit_handle obj, visit_handle *gln);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2
V_LIBSIMXPORT int simv2_UnstructuredMesh_check(visit_handle h);

#endif
