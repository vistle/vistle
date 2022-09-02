#include "portmanager.h"
#include "clustermanager.h"
#include <vistle/core/message.h>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <boost/lexical_cast.hpp>

namespace vistle {

PortManager::PortManager(ClusterManager *clusterManager): m_clusterManager(clusterManager)
{}

PortManager::~PortManager()
{}

const Port *PortManager::getPort(const int moduleID, const std::string &name)
{
    if (const Port *p = PortTracker::getPort(moduleID, name))
        return p;

    std::string::size_type p = name.find('[');
    if (p != std::string::npos) {
        Port *parent = findPort(moduleID, name.substr(0, p - 1));
        if (parent && (parent->flags() & Port::MULTI)) {
            size_t idx = boost::lexical_cast<size_t>(name.substr(p + 1));
            const Port *port = parent->child(idx);
            m_clusterManager->sendMessage(moduleID, message::AddPort(*port));
            return port;
        }
    }

    return NULL;
}

//! remove all connections to and from ports to a module
std::vector<message::Buffer> PortManager::removePort(const Port &port)
{
    std::vector<message::Buffer> msgs = PortTracker::removePort(port);
    for (const auto &msg: msgs)
        m_clusterManager->sendAll(msg);
    return msgs;
}

//! remove all connections to and from ports to a module
std::vector<message::Buffer> PortManager::removeModule(const int moduleID)
{
    std::vector<message::Buffer> msgs = PortTracker::removeModule(moduleID);
    for (const auto &msg: msgs)
        m_clusterManager->sendAll(msg);
    return msgs;
}

void PortManager::addObject(const Port *port)
{
    ++m_numObject[port];
}

bool PortManager::hasObject(const Port *port)
{
    return m_numObject[port] > 0;
}

void PortManager::popObject(const Port *port)
{
    assert(m_numObject[port] > 0);
    --m_numObject[port];
}

void PortManager::resetInput(const Port *port)
{
    ++m_numReset[port];
}

bool PortManager::isReset(const Port *port)
{
    return m_numReset[port] > 0;
}

void PortManager::popReset(const Port *port)
{
    --m_numReset[port];
}

void PortManager::finishInput(const Port *port)
{
    ++m_numFinish[port];
}

bool PortManager::isFinished(const Port *port)
{
    return m_numFinish[port] > 0;
}

void PortManager::popFinish(const Port *port)
{
    --m_numFinish[port];
}


} // namespace vistle
