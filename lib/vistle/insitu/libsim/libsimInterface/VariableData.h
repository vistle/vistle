// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_VARIABLEDATA_H
#define SIMV2_VARIABLEDATA_H
#include "export.h"
#include "VisItDataTypes.h"

#ifdef __cplusplus
extern "C" {
#endif
V_LIBSIMXPORT int simv2_VariableData_alloc(visit_handle *h);
V_LIBSIMXPORT int simv2_VariableData_free(visit_handle h);

V_LIBSIMXPORT int simv2_VariableData_setData(visit_handle h, int owner, int dataType, int nComps, int nTuples,
                                             void *data);

V_LIBSIMXPORT int simv2_VariableData_setDataEx(visit_handle h, int owner, int dataType, int nComps, int nTuples,
                                               void *data, void (*callback)(void *), void *callbackData);

V_LIBSIMXPORT int simv2_VariableData_setArrayData(visit_handle h, int arrIndex, int owner, int dataType, int nTuples,
                                                  int offset, int stride, void *data);

V_LIBSIMXPORT int simv2_VariableData_setDeletionCallback(visit_handle h, void (*callback)(void *), void *callbackData);

V_LIBSIMXPORT int simv2_VariableData_getData2(visit_handle h, int *owner, int *dataType, int *nComps, int *nTuples,
                                              void **data);

#ifdef __cplusplus
}
#endif

// These functions are only available in the runtime.
//owner: VISIT_OWNER_, dataType: VISIT_DATATYPE_ , nComps: ? , nTuples : dimension , data : nTuples * sizeof(dataType)
V_LIBSIMXPORT int simv2_VariableData_getData(visit_handle h, int &owner, int &dataType, int &nComps, int &nTuples,
                                             void *&data);

V_LIBSIMXPORT int simv2_VariableData_getDataEx(visit_handle h, int &owner, int &dataType, int &nComps, int &nTuples,
                                               void *&data, void (*&callback)(void *), void *&callbackData);

V_LIBSIMXPORT int simv2_VariableData_getNumArrays(visit_handle h, int *nArrays);

V_LIBSIMXPORT int simv2_VariableData_getArrayData(visit_handle h, int arrIndex, int &memory, int &owner, int &dataType,
                                                  int &nComps, int &nTuples, int &offset, int &stride, void *&data);

V_LIBSIMXPORT int simv2_VariableData_getDeletionCallback(visit_handle h, void (*&callback)(void *),
                                                         void *&callbackData);

V_LIBSIMXPORT int simv2_VariableData_nullData(visit_handle h);
#endif
