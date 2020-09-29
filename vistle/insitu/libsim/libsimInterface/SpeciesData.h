// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#ifndef SIMV2_SPECIESDATA_H
#define SIMV2_SPECIESDATA_H
#include "export.h"
#include "VisItDataTypes.h"
#include <vector>

// C-callable implementation of front end functions
#ifdef __cplusplus
extern "C" {
#endif

V_LIBSIMXPORT int simv2_SpeciesData_alloc(visit_handle *obj);
V_LIBSIMXPORT int simv2_SpeciesData_free(visit_handle obj);
V_LIBSIMXPORT int simv2_SpeciesData_addSpeciesName(visit_handle h, visit_handle);
V_LIBSIMXPORT int simv2_SpeciesData_setSpecies(visit_handle h, visit_handle);
V_LIBSIMXPORT int simv2_SpeciesData_setSpeciesMF(visit_handle h, visit_handle);
V_LIBSIMXPORT int simv2_SpeciesData_setMixedSpecies(visit_handle h, visit_handle);

#ifdef __cplusplus
}
#endif

// Callable from within the runtime and SimV2
V_LIBSIMXPORT int simv2_SpeciesData_getData(visit_handle h, std::vector<visit_handle> &speciesNames,
                                            visit_handle &species, visit_handle &speciesMF, visit_handle &mixedSpecies);

int simv2_SpeciesData_check(visit_handle h);

#endif
