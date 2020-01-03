#ifndef ESTABLISH_CONNECTION_H
#define ESTABLISH_CONNECTION_H

#include "EstablishConnectionExports.h"

#include <string>
#include <vector>
namespace in_situ {

bool V_LIBSIMCONNECTEXPORT readSim2File(const std::string& path, std::string& hostname, int& port, std::string& securityKey);
bool V_LIBSIMCONNECTEXPORT sendInitToSim(const std::vector<std::string> launchArgs, const std::string& host, int port, const std::string& key);
bool V_LIBSIMCONNECTEXPORT attemptLibSImConnection(const std::string& path, const std::vector<std::string>& args);
}


#endif // !ESTABLISH_CONNECTION_H
