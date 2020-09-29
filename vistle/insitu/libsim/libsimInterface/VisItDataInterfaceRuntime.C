// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#include "VisItDataInterfaceRuntime.h"
#include "VisItDataInterfaceRuntimeP.h"

#include "CSGMesh.h"
#include "CurveData.h"
#include "CurvilinearMesh.h"
#include "DomainBoundaries.h"
#include "DomainList.h"
#include "DomainNesting.h"
#include "MaterialData.h"
#include "PointMesh.h"
#include "RectilinearMesh.h"
#include "SimulationMetaData.h"
#include "SpeciesData.h"
#include "UnstructuredMesh.h"
#include "VariableData.h"

#include <stdlib.h>
#include <string>
#include <iostream>
#include <mutex>


#define ALLOC(T) (T *)calloc(1, sizeof(T))
#define FREE(PTR) \
    if (PTR != nullptr) \
    free(PTR)

//
// Define a structure that we can use to contain all of the callback function
// pointers that the user supplied.
//
typedef struct {
    /* Reader functions */
    int (*cb_ActivateTimestep)(void *);
    void *cbdata_ActivateTimestep;

    visit_handle (*cb_GetMetaData)(void *);
    void *cbdata_GetMetaData;

    visit_handle (*cb_GetMesh)(int, const char *, void *);
    void *cbdata_GetMesh;

    visit_handle (*cb_GetMaterial)(int, const char *, void *);
    void *cbdata_GetMaterial;

    visit_handle (*cb_GetSpecies)(int, const char *, void *);
    void *cbdata_GetSpecies;

    visit_handle (*cb_GetVariable)(int, const char *, void *);
    void *cbdata_GetVariable;

    visit_handle (*cb_GetMixedVariable)(int, const char *, void *);
    void *cbdata_GetMixedVariable;

    visit_handle (*cb_GetCurve)(const char *, void *);
    void *cbdata_GetCurve;

    visit_handle (*cb_GetDomainList)(const char *, void *);
    void *cbdata_GetDomainList;

    visit_handle (*cb_GetDomainBoundaries)(const char *, void *);
    void *cbdata_GetDomainBoundaries;

    visit_handle (*cb_GetDomainNesting)(const char *, void *);
    void *cbdata_GetDomainNesting;

    /* Writer functions */
    int (*cb_WriteBegin)(const char *, void *);
    void *cbdata_WriteBegin;

    int (*cb_WriteEnd)(const char *, void *);
    void *cbdata_WriteEnd;

    int (*cb_WriteMesh)(const char *, int, int, visit_handle, visit_handle, void *);
    void *cbdata_WriteMesh;

    int (*cb_WriteVariable)(const char *, const char *, int, visit_handle, visit_handle, void *);
    void *cbdata_WriteVariable;

} data_callback_t;

static std::mutex simv2_mutex;
typedef std::lock_guard<std::mutex> Guard;

static data_callback_t *visit_data_callbacks = nullptr;

static data_callback_t *GetDataCallbacks()
{
    if (visit_data_callbacks == nullptr)
        visit_data_callbacks = ALLOC(data_callback_t);
    return visit_data_callbacks;
}

void DataCallbacksCleanup(void)
{
    if (visit_data_callbacks != nullptr) {
        free(visit_data_callbacks);
        visit_data_callbacks = nullptr;
    }
}

// *****************************************************************************
// Data Interface functions (Callable from libsimV2)
// *****************************************************************************

void simv2_set_ActivateTimestep(int (*cb)(void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_ActivateTimestep = cb;
        callbacks->cbdata_ActivateTimestep = cbdata;
    }
}

void simv2_set_GetMetaData(visit_handle (*cb)(void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_GetMetaData = cb;
        callbacks->cbdata_GetMetaData = cbdata;
    }
}

void simv2_set_GetMesh(visit_handle (*cb)(int, const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_GetMesh = cb;
        callbacks->cbdata_GetMesh = cbdata;
    }
}

void simv2_set_GetMaterial(visit_handle (*cb)(int, const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_GetMaterial = cb;
        callbacks->cbdata_GetMaterial = cbdata;
    }
}

void simv2_set_GetSpecies(visit_handle (*cb)(int, const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_GetSpecies = cb;
        callbacks->cbdata_GetSpecies = cbdata;
    }
}

void simv2_set_GetVariable(visit_handle (*cb)(int, const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_GetVariable = cb;
        callbacks->cbdata_GetVariable = cbdata;
    }
}

void simv2_set_GetMixedVariable(visit_handle (*cb)(int, const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_GetMixedVariable = cb;
        callbacks->cbdata_GetMixedVariable = cbdata;
    }
}

void simv2_set_GetCurve(visit_handle (*cb)(const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_GetCurve = cb;
        callbacks->cbdata_GetCurve = cbdata;
    }
}

void simv2_set_GetDomainList(visit_handle (*cb)(const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_GetDomainList = cb;
        callbacks->cbdata_GetDomainList = cbdata;
    }
}

void simv2_set_GetDomainBoundaries(visit_handle (*cb)(const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_GetDomainBoundaries = cb;
        callbacks->cbdata_GetDomainBoundaries = cbdata;
    }
}

void simv2_set_GetDomainNesting(visit_handle (*cb)(const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_GetDomainNesting = cb;
        callbacks->cbdata_GetDomainNesting = cbdata;
    }
}

/* Write functions */
void simv2_set_WriteBegin(int (*cb)(const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_WriteBegin = cb;
        callbacks->cbdata_WriteBegin = cbdata;
    }
}

void simv2_set_WriteEnd(int (*cb)(const char *, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_WriteEnd = cb;
        callbacks->cbdata_WriteEnd = cbdata;
    }
}

void simv2_set_WriteMesh(int (*cb)(const char *, int, int, visit_handle, visit_handle, void *), void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_WriteMesh = cb;
        callbacks->cbdata_WriteMesh = cbdata;
    }
}

void simv2_set_WriteVariable(int (*cb)(const char *, const char *, int, visit_handle, visit_handle, void *),
                             void *cbdata)
{
    Guard g(simv2_mutex);
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr) {
        callbacks->cb_WriteVariable = cb;
        callbacks->cbdata_WriteVariable = cbdata;
    }
}

// *****************************************************************************
// Callable from SimV2 reader
// *****************************************************************************

// ****************************************************************************
// Method: simv2_invoke_ActivateTimeStep
//
// Purpose:
//   This function invokes the simulation's ActivateTimeStep callback function.
//
// Programmer: Brad Whitlock
// Creation:   Mon Mar 16 16:12:44 PDT 2009
//
// Modifications:
//
// ****************************************************************************

int simv2_invoke_ActivateTimestep(void)
{
    Guard g(simv2_mutex);
    int retval = VISIT_OKAY;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_ActivateTimestep != nullptr) {
        retval = (*callbacks->cb_ActivateTimestep)(callbacks->cbdata_ActivateTimestep);
    }
    return retval;
}

// ****************************************************************************
// Method: simv2_invoke_GetMetaData
//
// Purpose:
//   This function invokes the simulation's GetMetaData callback function and
//   returns a VisIt_SimulationMetaData structure that was populated by the
//   simulation.
//
// Arguments:
//
// Returns:
//
// Note:       This encapsulates the code to create an object ourselves,
//             pass it to the sim along with any user callback data that was
//             provided, and check the results for errors that could threaten
//             VisIt's operation.
//
// Programmer: Brad Whitlock
// Creation:   Thu Feb 26 16:03:44 PST 2009
//
// Modifications:
//
// ****************************************************************************

visit_handle simv2_invoke_GetMetaData(void)
{
    Guard g(simv2_mutex);
    visit_handle h = VISIT_INVALID_HANDLE;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_GetMetaData != nullptr) {
        h = (*callbacks->cb_GetMetaData)(callbacks->cbdata_GetMetaData);

        if (h != VISIT_INVALID_HANDLE) {
            if (simv2_ObjectType(h) != VISIT_SIMULATION_METADATA) {
                simv2_FreeObject(h);
                std::cerr << "The simulation returned a handle for an object other than simulation metadata."
                          << std::endl;
            }

            if (simv2_SimulationMetaData_check(h) == VISIT_ERROR) {
                simv2_FreeObject(h);
                std::cerr << "The metadata returned by the simulation did not pass a consistency check." << std::endl;
            }
        }
    }
    return h;
}

visit_handle simv2_invoke_GetMesh(int dom, const char *name)
{
    Guard g(simv2_mutex);
    visit_handle h = VISIT_INVALID_HANDLE;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_GetMesh != nullptr) {
        h = (*callbacks->cb_GetMesh)(dom, name, callbacks->cbdata_GetMesh);

        if (h != VISIT_INVALID_HANDLE) {
            int msgno = 0, err = VISIT_ERROR;
            int objType = simv2_ObjectType(h);
            switch (objType) {
            case VISIT_CSG_MESH:
                err = simv2_CSGMesh_check(h);
                break;
            case VISIT_CURVILINEAR_MESH:
                err = simv2_CurvilinearMesh_check(h);
                break;
            case VISIT_RECTILINEAR_MESH:
                err = simv2_RectilinearMesh_check(h);
                break;
            case VISIT_POINT_MESH:
                err = simv2_PointMesh_check(h);
                break;
            case VISIT_UNSTRUCTURED_MESH:
                err = simv2_UnstructuredMesh_check(h);
                break;
            default:
                msgno = 1;
            }

            if (err == VISIT_ERROR) {
                simv2_FreeObject(h);
                if (msgno == 0) {
                    std::cerr << "The mesh returned by the simulation did not pass a consistency check." << std::endl;
                } else {
                    std::cerr << "The simulation returned a handle for an object other than a mesh." << std::endl;
                }
            }
        }
    }
    return h;
}

visit_handle simv2_invoke_GetMaterial(int dom, const char *name)
{
    Guard g(simv2_mutex);
    visit_handle h = VISIT_INVALID_HANDLE;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_GetMaterial != nullptr) {
        h = (*callbacks->cb_GetMaterial)(dom, name, callbacks->cbdata_GetMaterial);

        if (h != VISIT_INVALID_HANDLE) {
            if (simv2_ObjectType(h) != VISIT_MATERIAL_DATA) {
                simv2_FreeObject(h);
                std::cerr << "The simulation returned a handle for an object other than a material data object."
                          << std::endl;
            }

            if (simv2_MaterialData_check(h) == VISIT_ERROR) {
                simv2_FreeObject(h);
                std::cerr << "The material returned by the simulation did not pass a consistency check." << std::endl;
            }
        }
    }
    return h;
}

visit_handle simv2_invoke_GetSpecies(int dom, const char *name)
{
    Guard g(simv2_mutex);
    visit_handle h = VISIT_INVALID_HANDLE;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_GetSpecies != nullptr) {
        h = (*callbacks->cb_GetSpecies)(dom, name, callbacks->cbdata_GetSpecies);

        if (h != VISIT_INVALID_HANDLE) {
            if (simv2_ObjectType(h) != VISIT_SPECIES_DATA) {
                simv2_FreeObject(h);
                std::cerr << "The simulation returned a handle for an object other than a species data object."
                          << std::endl;
            }

            if (simv2_SpeciesData_check(h) == VISIT_ERROR) {
                simv2_FreeObject(h);
                std::cerr << "The species returned by the simulation did not pass a consistency check." << std::endl;
            }
        }
    }
    return h;
}

visit_handle simv2_invoke_GetVariable(int dom, const char *name)
{
    Guard g(simv2_mutex);
    visit_handle h = VISIT_INVALID_HANDLE;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_GetVariable != nullptr) {
        h = (*callbacks->cb_GetVariable)(dom, name, callbacks->cbdata_GetVariable);

        if (h != VISIT_INVALID_HANDLE) {
            if (simv2_ObjectType(h) != VISIT_VARIABLE_DATA) {
                simv2_FreeObject(h);
                std::cerr << "The simulation returned a handle for an object other than a variable." << std::endl;
            }
        }
    }
    return h;
}

visit_handle simv2_invoke_GetMixedVariable(int dom, const char *name)
{
    Guard g(simv2_mutex);
    visit_handle h = VISIT_INVALID_HANDLE;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_GetMixedVariable != nullptr) {
        h = (*callbacks->cb_GetMixedVariable)(dom, name, callbacks->cbdata_GetMixedVariable);

        if (h != VISIT_INVALID_HANDLE) {
            if (simv2_ObjectType(h) != VISIT_VARIABLE_DATA) {
                simv2_FreeObject(h);
                std::cerr << "The simulation returned a handle for an object other than a variable." << std::endl;
            }
        }
    }
    return h;
}

visit_handle simv2_invoke_GetCurve(const char *name)
{
    Guard g(simv2_mutex);
    visit_handle h = VISIT_INVALID_HANDLE;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_GetCurve != nullptr) {
        h = (*callbacks->cb_GetCurve)(name, callbacks->cbdata_GetCurve);

        if (h != VISIT_INVALID_HANDLE) {
            if (simv2_ObjectType(h) != VISIT_CURVE_DATA) {
                simv2_FreeObject(h);
                std::cerr << "The simulation returned a handle for an object other than a curve." << std::endl;
            }

            if (simv2_CurveData_check(h) == VISIT_ERROR) {
                simv2_FreeObject(h);
                std::cerr << "The curve returned by the simulation did not pass a consistency check." << std::endl;
            }
        }
    }
    return h;
}

visit_handle simv2_invoke_GetDomainList(const char *name)
{
    Guard g(simv2_mutex);
    visit_handle h = VISIT_INVALID_HANDLE;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_GetDomainList != nullptr) {
        h = (*callbacks->cb_GetDomainList)(name, callbacks->cbdata_GetDomainList);

        if (h != VISIT_INVALID_HANDLE) {
            if (simv2_ObjectType(h) != VISIT_DOMAINLIST) {
                simv2_FreeObject(h);
                std::cerr << "The simulation returned a handle for an object other than a domain list." << std::endl;
            }

            if (simv2_DomainList_check(h) == VISIT_ERROR) {
                simv2_FreeObject(h);
                std::cerr << "The domain list returned by the simulation did not pass a consistency check."
                          << std::endl;
            }
        }
    }
    return h;
}

visit_handle simv2_invoke_GetDomainBoundaries(const char *name)
{
    Guard g(simv2_mutex);
    visit_handle h = VISIT_INVALID_HANDLE;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_GetDomainBoundaries != nullptr) {
        h = (*callbacks->cb_GetDomainBoundaries)(name, callbacks->cbdata_GetDomainBoundaries);
    }
    return h;
}

visit_handle simv2_invoke_GetDomainNesting(const char *name)
{
    Guard g(simv2_mutex);
    visit_handle h = VISIT_INVALID_HANDLE;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_GetDomainNesting != nullptr) {
        h = (*callbacks->cb_GetDomainNesting)(name, callbacks->cbdata_GetDomainNesting);
    }
    return h;
}

/* Writer functions. */

int simv2_invoke_WriteBegin(const char *name)
{
    Guard g(simv2_mutex);
    int ret = VISIT_ERROR;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_WriteBegin != nullptr)
        ret = (*callbacks->cb_WriteBegin)(name, callbacks->cbdata_WriteBegin);
    return ret;
}

int simv2_invoke_WriteEnd(const char *name)
{
    Guard g(simv2_mutex);
    int ret = VISIT_ERROR;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_WriteEnd != nullptr)
        ret = (*callbacks->cb_WriteEnd)(name, callbacks->cbdata_WriteEnd);
    return ret;
}

int simv2_invoke_WriteMesh(const char *name, int chunk, int meshType, visit_handle md, visit_handle mmd)
{
    Guard g(simv2_mutex);
    int ret = VISIT_ERROR;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_WriteMesh != nullptr)
        ret = (*callbacks->cb_WriteMesh)(name, chunk, meshType, md, mmd, callbacks->cbdata_WriteMesh);
    return ret;
}

int simv2_invoke_WriteVariable(const char *name, const char *arrName, int chunk, visit_handle data, visit_handle smd)
{
    Guard g(simv2_mutex);
    int ret = VISIT_ERROR;
    data_callback_t *callbacks = GetDataCallbacks();
    if (callbacks != nullptr && callbacks->cb_WriteVariable != nullptr)
        ret = (*callbacks->cb_WriteVariable)(name, arrName, chunk, data, smd, callbacks->cbdata_WriteVariable);
    return ret;
}

int simv2_ObjectType(visit_handle h)
{
    VisIt_ObjectBase *obj = VisItGetPointer(h);
    int t = -1;
    if (obj != nullptr)
        t = obj->objectType();
    return t;
}

int simv2_FreeObject(visit_handle h)
{
    int retval = VISIT_ERROR;
    VisIt_ObjectBase *obj = VisItGetPointer(h);
    if (obj != nullptr) {
        // Rely on the virtual destructor
        delete obj;
        VisItFreePointer(h);
        retval = VISIT_OKAY;
    }
    return retval;
}
