// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#include "VisItControlInterfaceRuntime.h"

#include "Engine.h"

#include "map"
#include "vector"
#include "cstring"

#include <iostream>

#include <vistle/insitu/libsim/libsimInterface/VisItDataInterfaceRuntime.h>

using vistle::insitu::libsim::Engine;


// ****************************************************************************
// Method: simv2_get_engine
//
// Purpose:
//   SimV2 runtime function to get the engine pointer, creating the object if
//   necessary.
//
// Returns:    A pointer to the engine.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Wed Sep 17 18:38:01 PDT 2014
//
// Modifications:
//
// ****************************************************************************

#if 0
// Engine creation callback.
static Engine *
simv2_create_engine(void *)
{
    return Engine::EngineInstance();
}
#endif

void *simv2_get_engine()
{
    // Set the engine creation callback so it will create our Engine subclass.
    //EngineBase::SetEngineCreationCallback(simv2_create_engine, NULL);

    //Engine::createEngine()->EnableSimulationPlugins();
    return (void *)Engine::EngineInstance();
}

// ****************************************************************************
// Method: simv2_initialize
//
// Purpose:
//   SimV2 runtime function to initialize the engine.
//
// Arguments:
//   e    : The engine pointer.
//   argc : The number of command line args.
//   argv : The command line args.
//   batch : True if we're initializing the engine for batch.
//
// Returns:    1 on success, 0 on failure.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Wed Sep 17 18:39:01 PDT 2014
//
// Modifications:
//
//    Brad Whitlock, Mon Aug 17 17:15:56 PDT 2015
//    Parse the command line options to allow plot and operator plugins to be
//    restricted.
//
//    Burlen Loring, Fri Oct  2 15:18:03 PDT 2015
//    Don't need to check the permissions on the .sim2 file.
//
// ****************************************************************************

static int simv2_initialize_helper(void *e, int argc, char *argv[], bool batch)
{
    Engine *engine = (Engine *)(e);
    return engine->initialize(argc, argv);
}

int simv2_initialize(void *e, int argc, char *argv[])
{
    return simv2_initialize_helper(e, argc, argv, false);
}

int simv2_initialize_batch(void *e, int argc, char *argv[])
{
    return simv2_initialize_helper(e, argc, argv, true);
}

// ****************************************************************************
// Method: simv2_connect_viewer
//
// Purpose:
//   Unnecessary, Vistle gui connects the renderer.
//
// Arguments:
//   e    : The engine pointer.
//   argc : The number of command line args.
//   argv : The command line args.
//
// Returns:    1 on success,
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Wed Sep 17 18:40:01 PDT 2014
//
// Modifications:
//
// ****************************************************************************

int simv2_connect_viewer(void *e, int argc, char *argv[])
{
    return true;
}

// ****************************************************************************
// Method: simv2_get_descriptor
//
// Purpose:
//   SimV2 runtime function to get the engine input socket descriptor.
//
// Arguments:
//   e : The engine pointer.
//
// Returns:    The engine's input socket.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   way back
//
// Modifications:
//
// ****************************************************************************

int simv2_get_descriptor(void *e)
{
    Engine *engine = (Engine *)(e);
    return engine->GetInputSocket();
    return -1;
}

// ****************************************************************************
// Method: simv2_process_input
//
// Purpose:
//   SimV2 runtime function to process input for the engine (commands from the
//   viewer socket).
//
// Arguments:
//   e : The engine pointer.
//
// Returns:    1 on success, 0 on failure.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   way back
//
// Modifications:
//
// ****************************************************************************

int simv2_process_input(void *e)
{
    Engine *engine = (Engine *)(e);

    return engine->fetchNewModuleState();
}

// ****************************************************************************
// Method: simv2_time_step_changed
//
// Purpose:
//   SimV2 runtime function called when the time step changes.
//
// Arguments:
//   e : The engine pointer.
//
// Returns:
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   way back
//
// Modifications:
//
// ****************************************************************************

void simv2_time_step_changed(void *e)
{
    Engine *engine = (Engine *)(e);
    engine->SimulationTimeStepChanged();
}

// ****************************************************************************
// Method: simv2_execute_command
//
// Purpose:
//   SimV2 runtime function called when we want to execute a command.
//
// Arguments:
//   e : The engine pointer.
//   command : A command string.
//
// Returns:
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   way back
//
// Modifications:
//
// ****************************************************************************

void simv2_execute_command(void *e, const char *command)
{
    if (command != NULL) {
        Engine *engine = (Engine *)(e);
        engine->SimulationInitiateCommand(command);
    }
}

// ****************************************************************************
// Method: simv2_disconnect
//
// Purpose:
//   SimV2 runtime function called when we want to disconnect from the simulation.
//
// Programmer: Brad Whitlock
// Creation:   way back
//
// Modifications:
//
// ****************************************************************************

void simv2_disconnect()
{
    Engine::DisconnectSimulation();


    DataCallbacksCleanup();
}

// ****************************************************************************
// Method: simv2_set_slave_process_callback
//
// Purpose:
//   SimV2 runtime function called when we want to install a slave process callback.
//
// Arguments:
//   spic : The new callback function.
//
// Returns:
//
// Note:       The slave process callback helps broadcast commands from the
//             viewer to other ranks.
//
// Programmer: Brad Whitlock
// Creation:   way back
//
// Modifications:
//
// ****************************************************************************

void simv2_set_slave_process_callback(void (*spic)())
{
    Engine::EngineInstance()->setSlaveComandCallback(spic);
}

// ****************************************************************************
// Method: simv2_set_command_callback
//
// Purpose:
//   SimV2 runtime function called when we want to install a callback to process
//   commands.
//
// Arguments:
//   e : The engine pointer.
//   sc : The command callback
//   scdata : The command callback data.
//
// Returns:
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   way back
//
// Modifications:
//
// ****************************************************************************

void simv2_set_command_callback(void *e, void (*sc)(const char *, const char *, void *), void *scdata)
{
    Engine *engine = (Engine *)(e);
    engine->SetSimulationCommandCallback(sc, scdata);
}

// ****************************************************************************
// Method: simv2_debug_logs
//
// Purpose:
//   SimV2 runtime function that adds a message to the debug logs.
//
// Arguments:
//   level : The debug level.
//   msg   : The message to write.
//
// Returns:
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Wed Sep 17 18:54:26 PDT 2014
//
// Modifications:
//
// ****************************************************************************

void simv2_debug_logs(int level, const char *msg)
{
    std::cerr << msg << std::endl;
}

// ****************************************************************************
// Method: simv2_set_mpicomm
//
// Purpose:
//   SimV2 runtime function that sets the MPI communicator.
//
// Arguments:
//   comm : The MPI communicator.
//
// Returns:    VISIT_OKAY, VISIT_ERROR
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Wed Sep 17 18:55:08 PDT 2014
//
// Modifications:
//
// ****************************************************************************

int simv2_set_mpicomm(void *comm)
{
    return Engine::EngineInstance()->setMpiComm(comm);
}

// ****************************************************************************
// Method: simv2_set_mpicomm_f
//
// Purpose:
//   SimV2 runtime function that sets the MPI communicator from Fortran which
//   uses an integer communicator handle.
//
// Arguments:
//   comm : The Fortran MPI communicator handle (integer).
//
// Returns:    VISIT_OKAY, VISIT_ERROR
//
// Note:
//
// Programmer: William T. Jones
// Creation:   Wed Sep 4 10:27:03 PDT 2013
//
// Modifications:
//
// ****************************************************************************

int simv2_set_mpicomm_f(int *comm)
{
#ifdef PARALLEL
    MPI_Fint *commF = (MPI_Fint *)comm;
    static MPI_Comm commC = MPI_Comm_f2c(*commF);
    return PAR_SetComm((void *)&commC) ? VISIT_OKAY : VISIT_ERROR;
#else
    return VISIT_ERROR;
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// THESE FUNCTIONS ARE MORE EXPERIMENTAL
///////////////////////////////////////////////////////////////////////////////

// ****************************************************************************
// Method: simv2_save_window
//
// Purpose:
//   SimV2 runtime function called when we want to save a window.
//
// Arguments:
//   e : The engine pointer.
//   filename : The filename to save to.
//   w : The image width
//   h : The image height
//   format : The image format.
//
// Returns:    VISIT_OKAY or VISIT_ERROR.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Wed Sep 17 18:52:03 PDT 2014
//
// Modifications:
//
// ****************************************************************************

int simv2_save_window(void *e, const char *filename, int w, int h, int format)
{
    return false;
}

// ****************************************************************************
// Method: simv2_add_plot
//
// Purpose:
//   SimV2 runtime function called when we want to add a plot
//
// Arguments:
//   e        : The engine pointer.
//   plotType : The plot type.
//   var      : The plot variable.
//
// Returns:    VISIT_OKAY or VISIT_ERROR.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Wed Sep 17 18:52:03 PDT 2014
//
// Modifications:
//
// ****************************************************************************

int simv2_add_plot(void *e, const char *plotType, const char *var)
{
    return false;
}

// ****************************************************************************
// Method: simv2_add_operator
//
// Purpose:
//   SimV2 runtime function called when we want to add a operator
//
// Arguments:
//   e            : The engine pointer.
//   operatorType : The operator type.
//   applyToAll   : Whether to apply the operator to all plots.
//
// Returns:    VISIT_OKAY or VISIT_ERROR.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Wed Sep 17 18:52:03 PDT 2014
//
// Modifications:
//
// ****************************************************************************

int simv2_add_operator(void *e, const char *operatorType, int applyToAll)
{
    return false;
}

// ****************************************************************************
// Method: simv2_draw_plots
//
// Purpose:
//   SimV2 runtime function called when we want to draw a plot.
//
// Arguments:
//   e : The engine pointer.
//
// Returns:
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 18 18:03:28 PDT 2014
//
// Modifications:
//
// ****************************************************************************

int simv2_draw_plots(void *e)
{
    Engine *engine = (Engine *)(e);
    return engine->sendData();
}

// ****************************************************************************
// Method: simv2_delete_active_plots
//
// Purpose:
//   SimV2 runtime function called when we want to delete the active plots.
//
// Arguments:
//   e : The engine pointer.
//
// Returns:
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 18 18:03:28 PDT 2014
//
// Modifications:
//
// ****************************************************************************

int simv2_delete_active_plots(void *e)
{
    Engine *engine = (Engine *)(e);
    engine->DeleteData();
    return true;
}

// ****************************************************************************
// Method: simv2_set_active_plots
//
// Purpose:
//   SimV2 function to set the active plots.
//
// Arguments:
//   e    : The engine pointer.
//   ids  : The list of plot ids.
//   nids : The number of plot ids.
//
// Returns:    OKAY on success, FALSE on failure.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Mon Feb  2 13:57:29 PST 2015
//
// Modifications:
//
// ****************************************************************************

int simv2_set_active_plots(void *e, const int *ids, int nids)
{
    return false;
}

// ****************************************************************************
// Method: simv2_change_plot_var
//
// Purpose:
//   SimV2 function to change the variable on plots.
//
// Arguments:
//   e    : The engine pointer.
//   var  : The new variable.
//   all  : Whether to do it on all plots or just the selected plots.
//
// Returns:    OKAY on success, FALSE on failure.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Mon Feb  2 13:57:29 PST 2015
//
// Modifications:
//
// ****************************************************************************

int simv2_change_plot_var(void *e, const char *var, int all)
{
    return false;
}

// ****************************************************************************
// Method: simv2_set_plot_options
//
// Purpose:
//   SimV2 function to set plot options.
//
// Arguments:
//   e : The engine pointer.
//   fieldName : The name of the field to set.
//   fieldType : The type of the data we're passing in.
//   fieldVal  : A pointer to the field data we're passing in.
//   fieldLen  : The length of the field data (if it is an array).
//
// Returns:    OKAY on success, FALSE on failure.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Mon Feb  2 14:07:11 PST 2015
//
// Modifications:
//
// ****************************************************************************

int simv2_set_plot_options(void *e, const char *fieldName, int fieldType, void *fieldVal, int fieldLen)
{
    return false;
}

// ****************************************************************************
// Method: simv2_set_operator_options
//
// Purpose:
//   SimV2 function to set operator options.
//
// Arguments:
//   e : The engine pointer.
//   fieldName : The name of the field to set.
//   fieldType : The type of the data we're passing in.
//   fieldVal  : A pointer to the field data we're passing in.
//   fieldLen  : The length of the field data (if it is an array).
//
// Returns:    OKAY on success, FALSE on failure.
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Mon Feb  2 14:07:11 PST 2015
//
// Modifications:
//
// ****************************************************************************

int simv2_set_operator_options(void *e, const char *fieldName, int fieldType, void *fieldVal, int fieldLen)
{
    return false;
}

// ****************************************************************************
// Method: simv2_exportdatabase
//
// Purpose:
//   SimV2 runtime function called when we want to execute a command.
//
// Arguments:
//   e        : The engine pointer.
//   filename : The filename to export.
//   format   : The export format.
//   names    : The list of variables to export.
//   options  : The optional options to use when exporting.
//
// Returns:    VISIT_OKAY on success; VISIT_ERROR on failure.
//
// Note:       EXPERIMENTAL
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 18 10:53:32 PDT 2014
//
// Modifications:
//
// ****************************************************************************

int simv2_exportdatabase_with_options(void *e, const char *filename, const char *format, visit_handle names,
                                      visit_handle options)
{
    return false;
}

// Left in for compatibility
int simv2_exportdatabase(void *e, const char *filename, const char *format, visit_handle names, visit_handle options)
{
    return simv2_exportdatabase_with_options(e, filename, format, names, VISIT_INVALID_HANDLE);
}

// ****************************************************************************
// Method: simv2_restoresession
//
// Purpose:
//   SimV2 runtime function called when we want to restore a session.
//
// Arguments:
//   e : The engine pointer.
//   filename : The session filename.
//
// Returns:
//
// Note:       EXPERIMENTAL
//
// Programmer: Brad Whitlock
// Creation:   Thu Sep 18 10:53:32 PDT 2014
//
// Modifications:
//
// ****************************************************************************

int simv2_restoresession(void *e, const char *filename)
{
    return false;
}

// ****************************************************************************
// Method: simv2_set_view2D
//
// Purpose:
//   Sets the view.
//
// Arguments:
//   e : The engine pointer.
//   v : A handle to a VisIt_View2D object.
//
// Returns:
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Thu Jun  1 17:22:28 PDT 2017
//
// Modifications:
//
// ****************************************************************************

int simv2_set_view2D(void *e, visit_handle v)
{
    return false;
}

int simv2_get_view2D(void *e, visit_handle v)
{
    return false;
}

// ****************************************************************************
// Method: simv2_set_view3D
//
// Purpose:
//   Sets the view.
//
// Arguments:
//   e : The engine pointer.
//   v : A handle to a VisIt_View3D object.
//
// Returns:
//
// Note:
//
// Programmer: Brad Whitlock
// Creation:   Thu Jun  1 17:22:28 PDT 2017
//
// Modifications:
//
// ****************************************************************************

int simv2_set_view3D(void *e, visit_handle v)
{
    return false;
}

int simv2_get_view3D(void *e, visit_handle v)
{
    return false;
}
