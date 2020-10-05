#ifndef VISTLE_SENSEI_H
#define VISTLE_SENSEI_H

#include "export.h"
#include "metaData.h"
#include "objectRetriever.h"

#include <mpi.h>
#include <thread>

namespace vistle {

namespace insitu {
namespace message {
class Message;
} // namespace message
namespace sensei {
namespace detail {
class Internals;
}
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
    typename T::ptr createVistleObject(Args &&... args) {
        if (isMOduleReady()) {
            auto obj = typename T::ptr(new T(args...));
            updateShmIDs();
            return obj;
        } else {
            return nullptr;
        }
    }

  private:
    ObjectRetriever m_callbacks;
    MetaData m_metaData;
    MetaData m_usedData;
    detail::Internals *m_internals = nullptr;

    bool m_connected = false; // If we are connected to the module
    size_t m_processedTimesteps = 0;
    // mpi info
    int m_rank = -1, m_mpiSize = 0;
    MPI_Comm comm = MPI_COMM_WORLD;

    bool m_ready = false;                   // true if the module is connected and executing
    std::map<std::string, bool> m_commands; // commands and their current state
#ifdef MODULE_THREAD
    std::thread m_managerThread;
    bool startVistle(const MPI_Comm &comm);
#endif
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
    bool isMOduleReady();
    void updateShmIDs();
};
} // namespace sensei
} // namespace insitu
} // namespace vistle

#endif // !VISTLE_SENSEI_H
