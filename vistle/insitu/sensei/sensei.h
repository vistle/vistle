#ifndef VISTLE_SENSEI_H
#define VISTLE_SENSEI_H

#include "export.h"
#include "intOption.h"
#include "metaData.h"
#include "objectRetriever.h"

#include <vistle/insitu/message/ShmMessage.h>
#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/insitu/message/addObjectMsq.h>

#include <mpi.h>

namespace vistle {
namespace message {
class MessageQueue;
} // namespace message

namespace insitu {
namespace sensei {

class V_SENSEIEXPORT SenseiAdapter //: public SenseiInterface

{
public:
  SenseiAdapter(bool paused, MPI_Comm Comm, MetaData &&meta, ObjectRetriever cbs);
  bool Execute(size_t timestep);
  bool Finalize();

  void operator=(SenseiAdapter &&other) = delete;
  void operator=(SenseiAdapter &other) = delete;
  SenseiAdapter(SenseiAdapter &&other) = delete;
  SenseiAdapter(SenseiAdapter &other) = delete;
  ~SenseiAdapter() = default;

  template <typename T, typename... Args>
  typename T::ptr createVistleObject(Args &&... args)
  {
    if (m_moduleInfo.isReady()) {
      return m_shmIDs.createVistleObject<T>(std::forward<Args>(args)...);
    } else {
      return nullptr;
    }
  }

private:
  ObjectRetriever m_callbacks;
  MetaData m_metaData;
  MetaData m_usedData;
  message::ModuleInfo m_moduleInfo;
  bool m_connected = false; // If we are connected to the module
  size_t m_processedTimesteps = 0;
  std::unique_ptr<vistle::insitu::message::AddObjectMsq>
      m_sendMessageQueue; // Queue to send addObject messages to module
  // mpi info
  int m_rank = -1, m_mpiSize = 0;
  MPI_Comm comm = MPI_COMM_WORLD;

  insitu::message::SyncShmIDs m_shmIDs;

  message::InSituShmMessage m_messageHandler;
  bool m_ready = false;                   // true if the module is connected and executing
  std::map<std::string, bool> m_commands; // commands and their current state
  std::set<IntOption> m_intOptions;       // options that can be set in the module

  bool stillConnected();
  bool quitRequested();
  bool WaitedForModuleCommands();
  bool haveToProcessTimestep(size_t timestep);

  void processData();

  void calculateUsedData();
  bool objectRequested(const std::string &name, const std::string &meshName = "");

  void dumpConnectionFile(MPI_Comm Comm); // create a file in which the sensei
                                          // module can find the connection info
  bool recvAndHandeMessage(bool blocking = false);
  bool initModule(const vistle::insitu::message::Message &msg);
  bool checkHostName() const;
  bool checkMpiSize() const;

  bool initializeVistleEnv();
  void initializeMessageQueues() throw();
  std::string portName(const std::string &meshName, const std::string &varName);

  void addCommands();
  void addPorts();
  void addObject(const std::string &port, vistle::Object::const_ptr obj);
};
} // namespace sensei
} // namespace insitu
} // namespace vistle

#endif // !VISTLE_SENSEI_H
