#include "LibSimControlModule.h"
#include <util/sleep.h>

ControllModule::ControllModule(const std::string& name, int moduleID, mpi::communicator comm)
    : vistle::Module("set ConnectLibSim in the state of prepare", name, moduleID, comm) {
    port = createOutputPort("noData", "");
    command = addStringParameter("command", "enter a command for the simulation", "");
    sendCommand = addIntParameter("sendCommand", "send the command to the simulation", false, vistle::Parameter::Boolean);

}


bool ControllModule::prepare() {
    while (true) {
        vistle::adaptive_wait(false, this);
        if (cancelRequested(true)) {
            break;
        }
    }
    return true;
}

MODULE_MAIN(ControllModule)
