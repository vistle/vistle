#include "porttracker.h"
#include "statetracker.h"
#include "parameter.h"
#include <vistle/core/message.h>
#include <iostream>
#include <algorithm>
#include <vistle/util/tools.h>

#define CERR std::cerr << "Port@" << (m_stateTracker ? m_stateTracker->m_name : "(null)") << ": "

namespace vistle {

PortTracker::PortTracker(): m_stateTracker(nullptr)
{}

PortTracker::~PortTracker()
{
    std::vector<int> modules;
    for (auto &m: m_ports) {
        modules.emplace_back(m.first);
    }
    for (auto id: modules) {
        removeModule(id);
    }
}

void PortTracker::setTracker(StateTracker *tracker)
{
    m_stateTracker = tracker;
}

StateTracker *PortTracker::tracker() const
{
    return m_stateTracker;
}

const Port *PortTracker::addPort(const int moduleID, const std::string &name, const std::string &description,
                                 const Port::Type type, int flags)
{
    assert(type != Port::ANY);

    PortMap *portMap = NULL;
    {
        std::map<int, PortMap *>::iterator i = m_ports.find(moduleID);
        if (i == m_ports.end()) {
            portMap = new PortMap;
            m_ports[moduleID] = portMap;
        } else {
            portMap = i->second;
        }
        assert(portMap);
    }

    PortOrder *portOrder = NULL;
    {
        std::map<int, PortOrder *>::iterator i = m_portOrders.find(moduleID);
        if (i == m_portOrders.end()) {
            portOrder = new PortOrder;
            m_portOrders[moduleID] = portOrder;
        } else {
            portOrder = i->second;
        }
        assert(portOrder);
    }

    PortMap::iterator pi = portMap->find(name);

    if (pi == portMap->end()) {
        if (type == Port::INPUT) {
            const bool combine = flags & Port::COMBINE_BIT;
            for (auto &p: *portMap) {
                if (p.second->getType() == Port::INPUT) {
                    if ((flags & Port::COMBINE_BIT) || combine) {
                        CERR << "COMBINE port has to be only input port of a module" << std::endl;
                        check();
                        return nullptr;
                    }
                }
            }
        }
        Port *port = new Port(moduleID, name, type, flags);
        port->setDescription(description);
        portMap->insert(std::make_pair(port->getName(), port));

        int paramNum = 0;
        const auto rit = portOrder->rbegin();
        if (rit != portOrder->rend())
            paramNum = rit->first + 1;
        (*portOrder)[paramNum] = port->getName();

        check();
        return port;
    } else {
        if (pi->second->getType() == Port::ANY) {
            pi->second->type = type;
            check();
            return pi->second;
        } else {
            check();
            return pi->second;
        }
    }

    return nullptr;
}

const Port *PortTracker::addPort(const Port &port)
{
    return addPort(port.getModuleID(), port.getName(), port.getDescription(), port.getType(), port.flags());
}

std::vector<message::Buffer> PortTracker::removePort(const Port &p)
{
    //CERR << "removing port " << p << std::endl;
    std::vector<message::Buffer> ret;

    const int moduleID = p.getModuleID();
    PortMap *portMap = NULL;
    {
        ModulePortMap::iterator i = m_ports.find(moduleID);
        if (i == m_ports.end()) {
            return ret;
        }
        portMap = i->second;
        assert(portMap);
    }

    std::map<int, PortOrder *>::iterator i = m_portOrders.find(moduleID);
    if (i != m_portOrders.end()) {
        PortOrder *portOrder = i->second;
        assert(portOrder);
        for (auto kv: *portOrder) {
            if (kv.second == p.getName()) {
                portOrder->erase(kv.first);
                break;
            }
        }
    }

    PortMap::iterator pi = portMap->find(p.getName());
    assert(pi != portMap->end());
    if (pi == portMap->end()) {
        return ret;
    }
    auto port = pi->second;
    assert(port);
    if (!port)
        return ret;

    check();
    const Port::ConstPortSet &cl = port->connections();
    while (!cl.empty()) {
        size_t oldsize = cl.size();
        const Port *other = *cl.begin();
        if (port->getType() != Port::INPUT && removeConnection(*port, *other)) {
            message::Disconnect d1(port->getModuleID(), port->getName(), other->getModuleID(), other->getName());
            ret.push_back(d1);
        }
        if (port->getType() != Port::OUTPUT && removeConnection(*other, *port)) {
            message::Disconnect d2(other->getModuleID(), other->getName(), port->getModuleID(), port->getName());
            ret.push_back(d2);
        }
        check();
        if (cl.size() == oldsize) {
            CERR << "failed to remove all connections for port " << *port << ", still left: " << cl.size() << std::endl;
            for (size_t i = 0; i < cl.size(); ++i) {
                CERR << "   " << *port << " <--> " << *other << std::endl;
            }
            break;
        }
    }
    portMap->erase(pi);
    delete port;
    check();

    return ret;
}

Port *PortTracker::findPort(const int moduleID, const std::string &name) const
{
    ModulePortMap::const_iterator i = m_ports.find(moduleID);

    if (i != m_ports.end()) {
        PortMap::iterator pi = i->second->find(name);
        if (pi != i->second->end())
            return pi->second;
    }

    return nullptr;
}

const Port *PortTracker::getPort(const int moduleID, const std::string &name)
{
    return findPort(moduleID, name);
#if 0

   std::map<int, PortMap *>::const_iterator i = m_ports.find(moduleID);

   if (i != m_ports.end()) {
      PortMap::iterator pi = i->second->find(name);
      if (pi != i->second->end())
         return pi->second;
   }

#if 0
   std::string::size_type p = name.find('[');
   if (p != std::string::npos) {
      Port *parent = getPort(moduleID, name.substr(0, p-1));
      if (parent && (parent->flags() & Port::MULTI)) {
         std::stringstream idxstr(name.substr(p+1));
         size_t idx=0;
         idxstr >> idx;
         Port *port = parent->child(idx);
         m_clusterManager->sendMessage(moduleID, message::CreatePort(port));
         return port;
      }
   }
#endif

   return NULL;
#endif
}

Port *PortTracker::findPort(const Port &p) const
{
    return findPort(p.getModuleID(), p.getName());
}

bool PortTracker::addConnection(const Port &from, const Port &to)
{
    Port *f = findPort(from);
    Port *t = findPort(to);

    assert(f);
    assert(t);

    bool added = false;
    if (f->getType() == Port::OUTPUT && t->getType() == Port::INPUT) {
        if (!t->isConnected() || (t->flags() & Port::COMBINE)) {
            if (f->addConnection(t)) {
                if (t->addConnection(f)) {
                    added = true;
                } else {
                    f->removeConnection(*t);
                    CERR << "PortTracker::addConnection: inconsistent connection states for data connection, from: "
                         << from << ", to: " << to << std::endl;
                    check();
                    return true;
                }
            }
        } else {
            //CERR << "PortTracker::addConnection: multiple connection" << std::endl;
            check();
            return true;
        }
    } else if (f->getType() == Port::PARAMETER && t->getType() == Port::PARAMETER) {
        if (f->addConnection(t)) {
            if (t->addConnection(f)) {
                added = true;
            } else {
                f->removeConnection(*t);
                CERR << "PortTracker::addConnection: inconsistent connection states for parameter connection, from: "
                     << from << ", to: " << to << std::endl;
                check();
                return true;
            }
        }
    } else {
        CERR << "PortTracker::addConnection: incompatible types: " << from << " (" << from.getType() << ") " << to
             << " (" << to.getType() << ")" << std::endl;

        check();
        return true;
    }

    if (added) {
        if (tracker()) {
            for (StateObserver *o: tracker()->m_observers) {
                o->incModificationCount();
                o->newConnection(f->getModuleID(), f->getName(), t->getModuleID(), t->getName());
            }
        }
        check();
        return true;
    }

#ifdef DEBUG
    CERR << "PortTracker::addConnection: have to queue connection: " << from->getModuleID() << ":" << from->getName()
         << " (" << from->getType() << ") " << to->getModuleID() << ":" << to->getName() << " (" << to->getType() << ")"
         << std::endl;
#endif

    check();
    return false;
}

bool PortTracker::addConnection(const int a, const std::string &na, const int b, const std::string &nb)
{
    Port *portA = findPort(a, na);
    if (!portA) {
        return false;
    }
    Port *portB = findPort(b, nb);
    if (!portB) {
        return false;
    }

    return addConnection(*portA, *portB);
}

bool PortTracker::removeConnection(const Port &from, const Port &to)
{
    //CERR << "removing connection: " << from << " -> " << to << std::endl;
    Port *f = findPort(from);
    if (!f)
        return false;

    Port *t = findPort(to);
    if (!t)
        return false;

    const auto ft = f->getType();
    const auto tt = t->getType();
    if (((ft == Port::OUTPUT || ft == Port::ANY) && (tt == Port::INPUT || tt == Port::ANY)) ||
        ((ft == Port::PARAMETER || ft == Port::ANY) && (tt == Port::PARAMETER || tt == Port::ANY))) {
        f->removeConnection(*t);
        t->removeConnection(*f);

        if (ft == Port::ANY || tt == Port::ANY) {
            CERR << "removeConnection: port type ANY: " << *f << " -> " << *t << std::endl;
        }

        if (tracker()) {
            for (StateObserver *o: tracker()->m_observers) {
                o->incModificationCount();
                o->deleteConnection(f->getModuleID(), f->getName(), t->getModuleID(), t->getName());
            }
        }

        check();
        return true;
    } else {
        CERR << "removeConnection: port type mismatch: " << *f << " -> " << *t << std::endl;
        f->removeConnection(*t);
        t->removeConnection(*f);
    }

    check();
    return false;
}

bool PortTracker::removeConnection(const int a, const std::string &na, const int b, const std::string &nb)
{
    const Port *from = findPort(a, na);
    const Port *to = findPort(b, nb);

    if (!from || !to)
        return false;

    return removeConnection(*from, *to);
}

const Port::ConstPortSet *PortTracker::getConnectionList(const Port *port) const
{
    return getConnectionList(port->getModuleID(), port->getName());
}

const Port::ConstPortSet *PortTracker::getConnectionList(const int moduleID, const std::string &name) const
{
    const Port *port = findPort(moduleID, name);
    if (!port)
        return NULL;
    return &port->connections();
}

std::vector<std::string> PortTracker::getPortNames(const int moduleID, Port::Type type) const
{
    std::vector<std::string> result;
    const auto ports = getPorts(moduleID, type);
    for (const auto &port: ports)
        result.push_back(port->getName());
    return result;
}

std::vector<std::string> PortTracker::getInputPortNames(const int moduleID) const
{
    return getPortNames(moduleID, Port::INPUT);
}

std::vector<std::string> PortTracker::getOutputPortNames(const int moduleID) const
{
    return getPortNames(moduleID, Port::OUTPUT);
}

std::string PortTracker::getPortDescription(const Port *port) const
{
    if (!port)
        return "";
    return port->description;
}

std::string PortTracker::getPortDescription(const int moduleID, const std::string &portname) const
{
    auto port = findPort(moduleID, portname);
    return getPortDescription(port);
}

std::vector<Port *> PortTracker::getPorts(const int moduleID, Port::Type type, bool connectedOnly) const
{
    std::vector<Port *> result;

    ModulePortMap::const_iterator mports = m_ports.find(moduleID);
    if (mports == m_ports.end())
        return result;
    const auto portOrderIt = m_portOrders.find(moduleID);
    if (portOrderIt == m_portOrders.end())
        return result;

    const PortMap &portmap = *mports->second;
    const PortOrder &portorder = *portOrderIt->second;
    for (PortOrder::const_iterator it = portorder.begin(); it != portorder.end(); ++it) {
        const std::string &name = it->second;
        auto it2 = portmap.find(name);
        if (it2 != portmap.end()) {
            assert(it2 != portmap.end());
            const auto &port = it2->second;

            if (port) {
                if (type == Port::ANY || port->getType() == type) {
                    if (!connectedOnly || !port->connections().empty()) {
                        result.push_back(port);
                    }
                }
            }
        }
    }

    return result;
}

std::vector<Port *> PortTracker::getInputPorts(const int moduleID) const
{
    return getPorts(moduleID, Port::INPUT);
}

std::vector<Port *> PortTracker::getConnectedInputPorts(const int moduleID) const
{
    return getPorts(moduleID, Port::INPUT, true);
}

std::vector<Port *> PortTracker::getOutputPorts(const int moduleID) const
{
    return getPorts(moduleID, Port::OUTPUT);
}

std::vector<Port *> PortTracker::getConnectedOutputPorts(const int moduleID) const
{
    return getPorts(moduleID, Port::OUTPUT, true);
}

std::vector<message::Buffer> PortTracker::removeModule(int moduleId)
{
    //CERR << "removing all connections from/to " << moduleId << std::endl;
    //check();

    std::vector<message::Buffer> ret;

    auto modulePortsIt = m_ports.find(moduleId);
    if (modulePortsIt != m_ports.end()) {
        std::vector<Port *> toRemove;
        const auto &modulePorts = *modulePortsIt->second;
        for (const auto &port: modulePorts) {
            toRemove.push_back(port.second);
        }

        for (auto p: toRemove) {
            auto r = removePort(*p);
            std::copy(r.begin(), r.end(), std::back_inserter(ret));
        }

        assert(modulePorts.empty());

        m_ports.erase(modulePortsIt);
    }

    if (m_ports.find(moduleId) != m_ports.end()) {
        CERR << "removeModule(" << moduleId << "): not yet removed" << std::endl;
    }

    check();

    for (const auto &mpm: m_ports) {
        const auto &pm = *mpm.second;
        for (const auto &pme: pm) {
            const Port *port = pme.second;
            if (port->getModuleID() == moduleId) {
                CERR << "removeModule " << moduleId << ": " << *port << " still exists" << std::endl;
            }
            const auto &cl = port->connections();
            for (const auto &other: cl) {
                if (other->getModuleID() == moduleId) {
                    CERR << "removeModule " << moduleId << ": " << *other << " still connected to " << *port
                         << std::endl;
                }
            }
        }
    }

    return ret;
}

bool PortTracker::check() const
{
    bool ok = true;
    int numInput = 0, numOutput = 0, numParam = 0;
    for (const auto &mpm: m_ports) {
        const auto &pm = *mpm.second;
        for (const auto &pme: pm) {
            const Port *port = pme.second;
            const auto &cl = port->connections();
            if (port->getType() == Port::INPUT && !(port->flags() & Port::COMBINE)) {
                if (cl.size() > 1) {
                    CERR << "FAIL: too many connections: " << *port << std::endl;
                }
            }
            const int moduleId = port->getModuleID();
            for (const auto &other: cl) {
                if (port->getType() == Port::ANY) {
                    CERR << "FAIL: ANY port: " << *port << std::endl;
                    ok = false;
                }
                if (other->getType() == Port::ANY) {
                    CERR << "FAIL: ANY port: " << *other << std::endl;
                    ok = false;
                }
                if (port->getType() == Port::INPUT) {
                    if (other->getType() != Port::OUTPUT) {
                        CERR << "FAIL: connection mismatch: " << *port << " <-> " << *other << std::endl;
                        ok = false;
                    } else {
                        ++numInput;
                    }
                    if (other->getModuleID() == moduleId) {
                        CERR << "FAIL: from/to same module: " << *port << " <-> " << *other << std::endl;
                        ok = false;
                    }
                } else if (port->getType() == Port::OUTPUT) {
                    if (other->getType() != Port::INPUT) {
                        CERR << "FAIL: connection mismatch: " << *port << " <-> " << *other << std::endl;
                        ok = false;
                    } else {
                        ++numOutput;
                    }
                    if (other->getModuleID() == moduleId) {
                        CERR << "FAIL: from/to same module: " << *port << " <-> " << *other << std::endl;
                        ok = false;
                    }
                } else if (port->getType() == Port::PARAMETER) {
                    if (other->getType() != Port::PARAMETER) {
                        CERR << "FAIL: connection mismatch: " << *port << " <-> " << *other << std::endl;
                        ok = false;
                    } else {
                        ++numParam;
                    }
                } else {
                    CERR << "FAIL: connection mismatch: " << *port << " <-> " << *other << std::endl;
                    ok = false;
                }
            }
        }
    }

    if (numInput != numOutput) {
        CERR << "FAIL: #connected inputs=" << numInput << ", #connected outputs=" << numOutput << " not equal"
             << std::endl;
        ok = false;
    }
    if (numParam % 2) {
        CERR << "FAIL: numParam=" << numParam << std::endl;
        ok = false;
    }
    if (!ok) {
        CERR << vistle::backtrace() << std::endl;
    }
    assert(ok);
    return ok;
}

} // namespace vistle
