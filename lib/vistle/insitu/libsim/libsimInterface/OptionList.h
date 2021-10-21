// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_OPTIONLIST_H
#define SIMV2_OPTIONLIST_H
#include "export.h"
#include "VisItDataTypes.h"

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C" {
#endif

V_LIBSIMXPORT int simv2_OptionList_alloc(visit_handle *obj);
V_LIBSIMXPORT int simv2_OptionList_free(visit_handle obj);
V_LIBSIMXPORT int simv2_OptionList_setValue(visit_handle obj, const char *name, int type, void *value);

V_LIBSIMXPORT int simv2_OptionList_getNumValues(visit_handle h, int *);
V_LIBSIMXPORT int simv2_OptionList_getIndex(visit_handle h, const char *, int *);
V_LIBSIMXPORT int simv2_OptionList_getType(visit_handle h, int, int *);
V_LIBSIMXPORT int simv2_OptionList_getName(visit_handle h, int, char **);
V_LIBSIMXPORT int simv2_OptionList_getValue(visit_handle h, int, void **);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2

#endif
