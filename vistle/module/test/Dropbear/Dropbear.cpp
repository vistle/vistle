#include <sstream>
#include <limits>
#include <algorithm>
#include <cstdlib>

#include <module/module.h>
#include <util/hostname.h>
#include <util/spawnprocess.h>
#include <util/sleep.h>

#include <sys/types.h>
#include <sys/wait.h>

using namespace vistle;

namespace {
std::vector<std::string> keytypes{"rsa", "dss", "ecdsa"};

}

class Dropbear: public vistle::Module {

 public:
   Dropbear(const std::string &name, int moduleID, mpi::communicator comm);
   ~Dropbear() override;

 private:

   bool compute() override;

   StringParameter *m_dbPath;
   StringParameter *m_dbOptions;
   IntParameter *m_dbPort;

   std::vector<StringParameter *> m_dbKey;

   IntParameter *m_exposedRank;
   IntParameter *m_exposedPort;
};

using namespace vistle;

Dropbear::Dropbear(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("Dropbear SSH server", name, moduleID, comm)
{

   std::stringstream str;
   str << "ctor: rank " << rank() << "/" << size() << " on host " << hostname() << std::endl;
   sendInfo(str.str());

   m_dbPath = addStringParameter("dropbear_path", "pathname to dropbear binary", "dropbear");
   m_dbOptions = addStringParameter("dropbear_options", "additional dropbear arguments", "-F -s -g");
   m_dbPort = addIntParameter("dropbear_port", "port where dropbear should listen (passed as argument to -p)", 31222);

   for (auto type: keytypes) {
       auto param = addStringParameter("hostkey_"+type, "path to "+type+" host key (~/.ssh/vistle_dropbear_"+type+")", "", Parameter::Filename);
       m_dbKey.push_back(param);
   }

   m_exposedRank = addIntParameter("exposed_rank", "rank to expose on hub (-1: none)", 0);
   setParameterRange(m_exposedRank, (Integer)-1, (Integer)size()-1);

   m_exposedPort = addIntParameter("exposed_port", "port to which exposed rank's dropbear should be forwarded to on hub (<=0: absolute value used as offset to dropbear port)", -1);
   setParameterRange(m_exposedPort, -(Integer)std::numeric_limits<unsigned short>::max(), (Integer)std::numeric_limits<unsigned short>::max());
}

Dropbear::~Dropbear() = default;

bool Dropbear::compute() {

   std::stringstream str;

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

   str << m_dbPath->getValue() << " " << m_dbOptions->getValue() << " -p " << m_dbPort->getValue();
   auto it = m_dbKey.begin();
   for (auto type: keytypes) {
       std::string key = (*it)->getValue();
       if (key.empty())
           key = home + "/.ssh/vistle_dropbear_" + type;
       str << " -r " << key;
       ++it;
   }

   std::string host = hostname().c_str();
   std::string command = str.str();
   sendInfo("Executing %s on %s...", command.c_str(), host.c_str());

   std::vector<std::string> args {"sh", "-c", command};
   process_handle pid = spawn_process("/bin/sh", args);

   int status = 0;
   while (try_wait(pid, &status) == 0) {
       if (cancelRequested()) {
           kill_process(pid);
       }
       adaptive_wait(false, this);
   }

   if (WIFEXITED(status)) {
       sendInfo("Exit with status %d on %s", WEXITSTATUS(status), host.c_str());
   } else if (WIFSIGNALED(status)) {
       sendInfo("Exit with signal %d on %s", WTERMSIG(status), host.c_str());
   } else {
       sendWarning("Unknown exit state %d on %s", status, host.c_str());
   }

   if (rank() == exposed) {
       removePortMapping(port);
   }

   return true;
}

MODULE_MAIN(Dropbear)

