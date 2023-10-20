#ifndef VISTLE_SENSEI_H
#define VISTLE_SENSEI_H

#include "export.h"
#include "metaData.h"
#include "objectRetriever.h"
#include <vistle/util/stopwatch.h>

#include <mpi.h>
#include <thread>

namespace vistle {

namespace insitu {
namespace message {
class Message;
} // namespace message
namespace sensei {
namespace detail {
struct Internals;
}
class V_SENSEIEXPORT Adapter //: public SenseiInterface
{
public:
    Adapter(bool paused, MPI_Comm Comm, MetaData &&meta, ObjectRetriever cbs, const std::string &vistleRoot,
            const std::string &vistleBuildType, const std::string &options);
    bool Execute(size_t timestep);
    bool Finalize();

    Adapter &operator=(Adapter &&other) = delete;
    Adapter &operator=(Adapter &other) = delete;
    Adapter(Adapter &&other) = delete;
    Adapter(Adapter &other) = delete;
    ~Adapter();

    template<typename T, typename... Args>
    typename T::ptr createVistleObject(Args &&...args)
    {
        return typename T::ptr(new T(args...));
    }
    void updateMeta(vistle::Object::ptr obj) const;

private:
    ObjectRetriever m_callbacks;
    MetaData m_metaData;
    MetaData m_usedData;
    detail::Internals *m_internals = nullptr;
    MPI_Comm m_comm;
    bool m_connected = false; // If we are connected to the module
    size_t m_processedTimesteps = 0;
    size_t m_iterations = 0;
    size_t m_executionCount = 0;
    // mpi info
    int m_rank = -1, m_mpiSize = 0;
    MPI_Comm comm = MPI_COMM_WORLD;
    double m_timeSpendInExecute = 0;
    double m_startTime = 0;
    std::map<std::string, bool> m_commands; // commands and their current state
#ifdef MODULE_THREAD
    std::thread m_managerThread;
    bool startVistle(const MPI_Comm &comm, const std::string &options);
#endif
    bool stillConnected();
    void restart();
    bool waitedForModuleCommands();
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
};
} // namespace sensei
} // namespace insitu
} // namespace vistle

#endif // !VISTLE_SENSEI_H
