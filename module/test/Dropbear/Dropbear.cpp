#include <sstream>
#include <limits>
#include <algorithm>
#include <cstdlib>

#include <boost/process.hpp>
#include <boost/algorithm/string.hpp>

#include <vistle/module/module.h>
#include <vistle/util/hostname.h>
#include <vistle/util/sleep.h>

using namespace vistle;
namespace process = boost::process;

namespace {
std::vector<std::string> keytypes{"rsa", "dss", "ecdsa"};

}

class Dropbear: public vistle::Module {
public:
    Dropbear(const std::string &name, int moduleID, mpi::communicator comm);
    ~Dropbear() override;

private:
    bool prepare() override;

    StringParameter *m_dbPath;
    StringParameter *m_dbOptions;
    IntParameter *m_dbPort;

    std::vector<StringParameter *> m_dbKey;

    IntParameter *m_exposedRank;
    IntParameter *m_exposedPort;
};

using namespace vistle;

Dropbear::Dropbear(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    std::stringstream str;
    str << "ctor: rank " << rank() << "/" << size() << " on host " << hostname() << std::endl;
    sendInfo(str.str());

    m_dbPath = addStringParameter("dropbear_path", "pathname to dropbear binary", "dropbear");
    m_dbOptions = addStringParameter("dropbear_options", "additional dropbear arguments", "-F -s -g");
    m_dbPort = addIntParameter("dropbear_port", "port where dropbear should listen (passed as argument to -p)", 31222);

    for (auto type: keytypes) {
        auto param =
            addStringParameter("hostkey_" + type, "path to " + type + " host key (~/.ssh/vistle_dropbear_" + type + ")",
                               "", Parameter::Filename);
        m_dbKey.push_back(param);
    }

    m_exposedRank = addIntParameter("exposed_rank", "rank to expose on hub (-1: none)", 0);
    setParameterRange(m_exposedRank, (Integer)-1, (Integer)size() - 1);

    m_exposedPort = addIntParameter("exposed_port",
                                    "port to which exposed rank's dropbear should be forwarded to on hub (<=0: "
                                    "absolute value used as offset to dropbear port)",
                                    -1);
    setParameterRange(m_exposedPort, -(Integer)std::numeric_limits<unsigned short>::max(),
                      (Integer)std::numeric_limits<unsigned short>::max());
}

Dropbear::~Dropbear() = default;

bool Dropbear::prepare()
{
    std::string home;
    if (auto h = getenv("HOME"))
        home = h;

    int port = m_exposedPort->getValue();
    if (port <= 0)
        port = m_dbPort->getValue() - port;

    int exposed = m_exposedRank->getValue();
    if (rank() == exposed) {
        requestPortMapping(port, m_dbPort->getValue());
    }

    std::string cmd = m_dbPath->getValue();
    std::vector<std::string> args;
    boost::algorithm::split(args, m_dbOptions->getValue(), boost::is_any_of(" \t"));
    args.push_back("-p");
    args.push_back(std::to_string(m_dbPort->getValue()));

    auto it = m_dbKey.begin();
    for (auto type: keytypes) {
        std::string key = (*it)->getValue();
        if (key.empty())
            key = home + "/.ssh/vistle_dropbear_" + type;
        args.push_back("-r");
        args.push_back(key);
        ++it;
    }

    std::stringstream str;
    str << "\"" << cmd;
    for (const auto &a: args)
        str << " " << a;
    str << "\"";

    std::string host = hostname().c_str();
    std::string command = str.str();
    sendInfo("Executing %s on %s...", command.c_str(), host.c_str());

    auto child = process::child(process::search_path(cmd), process::args(args));
    if (!child.valid()) {
        sendError("Failed to start dropbear on host %s", host.c_str());
    } else {
        while (child.running()) {
            if (cancelRequested()) {
                child.terminate();
                break;
            }
            adaptive_wait(false, this);
        }
        sendInfo("Exit with status %d on %s", child.exit_code(), host.c_str());
    }

    if (rank() == exposed) {
        removePortMapping(port);
    }

    return true;
}

MODULE_MAIN(Dropbear)
