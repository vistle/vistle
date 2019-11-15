// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_DOMAINNESTING_H
#define SIMV2_DOMAINNESTING_H
#include "VisItExports.h"
#include "VisItDataTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

V_VISITXPORT int simv2_DomainNesting_alloc(visit_handle*);
V_VISITXPORT int simv2_DomainNesting_free(visit_handle);
V_VISITXPORT int simv2_DomainNesting_set_dimensions(visit_handle,int,int,int);
V_VISITXPORT int simv2_DomainNesting_set_levelRefinement(visit_handle, int, int[3]);
V_VISITXPORT int simv2_DomainNesting_set_nestingForPatch(visit_handle, int, int, const int *, int, int[6]);

/* This function is only available in the runtime. */
V_VISITXPORT void *simv2_DomainNesting_avt(visit_handle);

#ifdef __cplusplus
}
#endif

#endif
