#ifndef ESTABLISH_CONNECTION_H
#define ESTABLISH_CONNECTION_H

#include "export.h"

#include <string>
#include <vector>
namespace vistle {
namespace insitu {
namespace libsim {
// read a hostname, port and security key out of a .sim2 file from a LibSim simulation
bool V_LIBSIMCONNECTEXPORT readSim2File(const std::string &path, std::string &hostname, int &port,
                                        std::string &securityKey);
// send a tcp message with the launch args to the simulation.
// For single process vistle these args are used to start athe vistle manager in the simulation process
// for multi process vistle they are used to create the ConnectLibSim module in the simulation process
bool V_LIBSIMCONNECTEXPORT sendInitToSim(const std::vector<std::string> launchArgs, const std::string &host, int port,
                                         const std::string &key);
bool V_LIBSIMCONNECTEXPORT attemptLibSimConnection(const std::string &path, const std::vector<std::string> &args);
} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif // !ESTABLISH_CONNECTION_H
