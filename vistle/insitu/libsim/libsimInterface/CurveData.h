// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_CURVEDATA_H
#define SIMV2_CURVEDATA_H
#include "export.h"
#include "VisItDataTypes.h"

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C"
{
#endif

V_LIBSIMXPORT int simv2_CurveData_alloc(visit_handle*);
V_LIBSIMXPORT int simv2_CurveData_free(visit_handle);

V_LIBSIMXPORT int simv2_CurveData_setCoordsXY(visit_handle obj, visit_handle x, visit_handle y);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2
V_LIBSIMXPORT int simv2_CurveData_getData(visit_handle h, 
                                      visit_handle &x,
                                      visit_handle &y);

V_LIBSIMXPORT int simv2_CurveData_check(visit_handle h);

#endif
