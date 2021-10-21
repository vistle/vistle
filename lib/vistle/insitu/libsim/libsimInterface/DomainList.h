// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_DOMAINLIST_H
#define SIMV2_DOMAINLIST_H
#include "export.h"
#include "VisItDataTypes.h"

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C" {
#endif

V_LIBSIMXPORT int simv2_DomainList_alloc(visit_handle *);
V_LIBSIMXPORT int simv2_DomainList_free(visit_handle);

V_LIBSIMXPORT int simv2_DomainList_setDomains(visit_handle obj, int alldoms, visit_handle mydoms);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2
V_LIBSIMXPORT int simv2_DomainList_getData(visit_handle h, int &alldoms, visit_handle &mydoms);

V_LIBSIMXPORT int simv2_DomainList_check(visit_handle h);

#endif
