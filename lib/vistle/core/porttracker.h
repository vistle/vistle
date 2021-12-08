#ifndef PORTTRACKER_H
#define PORTTRACKER_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <iostream>

#include "export.h"
#include "port.h"
#include "message.h"

namespace vistle {

class StateTracker;

class V_COREEXPORT PortTracker {
public:
    PortTracker();
    virtual ~PortTracker();
    void setTracker(StateTracker *tracker);
    StateTracker *tracker() const;

    const Port *addPort(const int moduleID, const std::string &name, const std::string &description,
                        const Port::Type type, int flags = 0);
    const Port *addPort(const Port &port);
    virtual std::vector<message::Buffer> removePort(const Port &port);

    bool addConnection(const Port &out, const Port &in);
    bool addConnection(const int a, const std::string &na, const int b, const std::string &nb);

    bool removeConnection(const Port &from, const Port &to);
    bool removeConnection(const int a, const std::string &na, const int b, const std::string &nb);

    typedef std::vector<const Port *> ConnectionList;

    const Port::ConstPortSet *getConnectionList(const Port *port) const;
    const Port::ConstPortSet *getConnectionList(const int moduleID, const std::string &name) const;

    virtual const Port *getPort(const int moduleID, const std::string &name);
    Port *findPort(const Port &p) const;
    Port *findPort(const int moduleID, const std::string &name) const;

    std::vector<std::string> getPortNames(const int moduleID, Port::Type type) const;
    std::vector<std::string> getInputPortNames(const int moduleID) const;
    std::vector<std::string> getOutputPortNames(const int moduleID) const;

    std::vector<std::string> getPortDescriptions(const int moduleID, Port::Type type) const;
    std::vector<std::string> getInputPortDescriptions(const int moduleID) const;
    std::vector<std::string> getOutputPortDescriptions(const int moduleID) const;

    std::vector<Port *> getPorts(const int moduleID, Port::Type type, bool connectedOnly = false) const;
    std::vector<Port *> getInputPorts(const int moduleID) const;
    std::vector<Port *> getConnectedInputPorts(const int moduleID) const;
    std::vector<Port *> getOutputPorts(const int moduleID) const;
    std::vector<Port *> getConnectedOutputPorts(const int moduleID) const;

    virtual std::vector<message::Buffer> removeModule(int moduleId);

protected:
    bool check() const;


    StateTracker *m_stateTracker;

    typedef std::map<std::string, Port *> PortMap;
    // module ID -> list of ports belonging to the module
    typedef std::map<int, PortMap *> ModulePortMap;
    ModulePortMap m_ports;

    typedef std::map<int, std::string> PortOrder;
    std::map<int, PortOrder *> m_portOrders;
};
} // namespace vistle

#endif
