#include "connect.h"

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>

namespace asio = boost::asio;
using namespace vistle::insitu::libsim;

static asio::io_service m_ioService;

bool vistle::insitu::libsim::readSim2File(const std::string &path, std::string &hostname, int &port,
                                          std::string &securityKey)
{
    std::ifstream f;
    f.open(path.c_str());
    if (!f.is_open()) {
        std::cerr << "ConnectLibSim::readSim2File: invalid file path: " << path << std::endl;
        return false;
    }
    std::string token;
    int i = 0;
    while (i < 3 && !f.eof()) {
        f >> token;
        if (token == "host") {
            f >> hostname;
            ++i;
        } else if (token == "port") {
            f >> port;
            ++i;
        } else if (token == "key") {
            f >> securityKey;
            ++i;
        }
    }
    f.close();
    if (i < 3) {
        std::cerr << "readSim2File: wrong file format" << std::endl;
        return false;
    }
    return true;
}

bool vistle::insitu::libsim::sendInitToSim(const std::vector<std::string> launchArgs, const std::string &host, int port,
                                           const std::string &key)
{
    //
    // Create a socket.
    //
    asio::io_service ios;
    boost::system::error_code ec;
    std::unique_ptr<boost::asio::ip::tcp::socket> s;
    asio::ip::tcp::resolver resolver(ios);
    asio::ip::tcp::resolver::query query(host, std::to_string(port), asio::ip::tcp::resolver::query::numeric_service);
    s.reset(new boost::asio::ip::tcp::socket(m_ioService));
    asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
    if (ec) {
        std::cerr << "SendInitToSim failed to resolve query to host " << host << " on port " << port << ": "
                  << ec.message() << std::endl;
        return false;
    }
    asio::connect(*s, endpoint_iterator, ec);
    if (ec) {
        std::cerr << "SendInitToSim failed to connect socket to host " << host << " on port " << port << ": "
                  << ec.message() << std::endl;
        return false;
    }

    //
    // Send the security key and launch information to the simulation
    //
    constexpr size_t bufferSize = 500;
    char tmp[bufferSize];

    memset(tmp, 0, sizeof(char) * bufferSize);
    sprintf(tmp, "%s\n", key.c_str());
    asio::write(*s, asio::buffer(std::string(tmp)), ec);
    if (ec) {
        std::cerr << "failed to send security key to sim" << std::endl;
        return false;
    }

    //
    // Receive a reply
    //

    strcpy(tmp, "");
    boost::asio::streambuf streambuf;
    auto n = asio::read_until(*s, streambuf, '\n', ec);
    if (ec) {
        std::cerr << "failed to read response from sim" << std::endl;
        return false;
    }
    streambuf.commit(n);

    std::istream is(&streambuf);
    std::string rspns;
    is >> rspns;
    if (rspns != "success") {
        std::cerr << "SendInitToSim: simulation did not connect" << std::endl;
        return false;
    }
    std::cerr << "SendInitToSim: simulation return success" << std::endl;
    // Create the Launch args
    memset(tmp, 0, sizeof(char) * bufferSize);
    strcpy(tmp, "");
    for (size_t i = 0; i < launchArgs.size(); i++) {
        strcat(tmp, launchArgs[i].c_str());
        strcat(tmp, "\n");
    }
    strcat(tmp, "\n");

    // Send it!
    asio::write(*s, asio::buffer(std::string(tmp)), ec);
    if (ec) {
        std::cerr << "failed to send args to sim" << std::endl;
        return false;
    }
    return true;
}

bool vistle::insitu::libsim::attemptLibSimConnection(const std::string &path, const std::vector<std::string> &args)
{
    int port;
    std::string host, key;
    if (!vistle::insitu::libsim::readSim2File(path, host, port, key)) {
        return false;
    }
    return vistle::insitu::libsim::sendInitToSim(args, host, port, key);
}
