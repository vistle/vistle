#ifndef PORTMANAGER_H
#define PORTMANAGER_H

#include <string>

#include <vistle/core/porttracker.h>

namespace vistle {

class ClusterManager;

class PortManager: public PortTracker {
public:
    PortManager(ClusterManager *clusterManager);
    virtual ~PortManager();

    virtual const Port *getPort(const int moduleID, const std::string &name) override;
    std::vector<message::Buffer> removePort(const Port &port) override;
    std::vector<message::Buffer> removeModule(int moduleId) override;

    void addObject(const Port *port);
    bool hasObject(const Port *port);
    void popObject(const Port *port);

    void resetInput(const Port *port);
    bool isReset(const Port *port);
    void popReset(const Port *port);

    void finishInput(const Port *port);
    bool isFinished(const Port *port);
    void popFinish(const Port *port);

private:
    ClusterManager *m_clusterManager;

    std::map<const Port *, int> m_numObject, m_numReset, m_numFinish;
};
} // namespace vistle

#endif
