#include "LibSimControlModule.h"
#include <util/sleep.h>

ControllModule::ControllModule(const std::string& name, int moduleID, mpi::communicator comm)
    : vistle::Reader("set ConnectLibSim in the state of prepare", name, moduleID, comm) {
    port = createOutputPort("noData", "");
    command = addStringParameter("command", "enter a command for the simulation", "");
    sendCommand = addIntParameter("sendCommand", "send the command to the simulation", false, vistle::Parameter::Boolean);
    observeParameter(command);
    observeParameter(sendCommand);

    setPartitions(1);
    setTimesteps(0);
}



void ControllModule::sendComandToSim() {
    std::cerr << "sending command: " << command->getValue() << std::endl;
}

bool ControllModule::examine(const vistle::Parameter* param) {
    if (sendCommand->getValue() != sendCommandChanged) {
        sendCommandChanged = sendCommand->getValue();
        sendComandToSim();
    }
    return true;
}

bool ControllModule::read(Token& token, int timestep, int block) {
    while (true) {
        vistle::adaptive_wait(false, this);
        if (cancelRequested(true)) {
            break;
        }
    }
    return true;
}

MODULE_MAIN(ControllModule)
