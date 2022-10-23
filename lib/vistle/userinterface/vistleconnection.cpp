/*********************************************************************************/
/*! \file handler.cpp
 *
 * Contains three classes:
 * 1. VistleConnection -- simple class, handling commands dispatched to vistle's userInterface.
 * 2. VistleObserver -- observer class that watches for changes in vistle, and sends
 *    signals to the MainWindow.
 */
/**********************************************************************************/
#include "vistleconnection.h"
#include "userinterface.h"
#include <vistle/util/threadname.h>

namespace vistle {

VistleConnection *VistleConnection::s_instance = nullptr;

/*************************************************************************/
// begin class VistleConnection
/*************************************************************************/

VistleConnection::VistleConnection(vistle::UserInterface &ui): m_ui(ui), m_done(false), m_quitOnExit(false)
{
    assert(s_instance == nullptr);
    s_instance = this;
}

VistleConnection::~VistleConnection()
{
    s_instance = nullptr;
}

VistleConnection &VistleConnection::the()
{
    assert(s_instance);
    return *s_instance;
}

bool VistleConnection::done() const
{
    mutex_lock lock(m_mutex);
    return m_done;
}

bool VistleConnection::started() const
{
    mutex_lock lock(m_mutex);
    return m_started;
}

void VistleConnection::cancel()
{
    if (!done()) {
        mutex_lock lock(m_mutex);
        m_done = true;
    }

    if (m_quitOnExit) {
        if (ui().isConnected()) {
            sendMessage(message::Quit());
            m_quitOnExit = false;
        }
    }

    mutex_lock lock(m_mutex);
    ui().stop();
}

void VistleConnection::operator()()
{
    setThreadName("VistleConnection");
    mutex_lock lock(m_mutex);
    m_started = true;
    lock.unlock();
    while (m_ui.dispatch()) {
        lock.lock();
        if (m_done) {
            lock.unlock();
            break;
        }
        lock.unlock();
    }
    lock.lock();
    m_done = true;
}

void VistleConnection::setQuitOnExit(bool quit)
{
    m_quitOnExit = quit;
}

vistle::UserInterface &VistleConnection::ui() const
{
    return m_ui;
}

void VistleConnection::lock()
{
    m_mutex.lock();
}

void VistleConnection::unlock()
{
    m_mutex.unlock();
}

bool VistleConnection::sendMessage(const vistle::message::Message &msg, const buffer *payload) const
{
    mutex_lock lock(m_mutex);
    return ui().sendMessage(msg, payload);
}

std::shared_ptr<vistle::Parameter> VistleConnection::getParameter(int id, const std::string &name) const
{
    mutex_lock lock(m_mutex);
    auto p = ui().state().getParameter(id, name);
    if (!p) {
        std::cerr << "no such parameter: " << id << ":" << name << std::endl;
    }
    return p;
}

bool vistle::VistleConnection::sendParameter(const std::shared_ptr<Parameter> p) const
{
    mutex_lock lock(m_mutex);
    vistle::message::SetParameter set(p->module(), p->getName(), p);
    set.setDestId(p->module());
    if (p->isImmediate())
        set.setPriority(vistle::message::Message::ImmediateParameter);
    return sendMessage(set);
}

bool VistleConnection::requestReplyAsync(const vistle::message::Message &send) const
{
    if (!ui().getLockForMessage(send.uuid()))
        return false;

    return sendMessage(send);
}

bool VistleConnection::waitForReplyAsync(const vistle::message::uuid_t &uuid, vistle::message::Message &reply) const
{
    return ui().getMessage(uuid, reply);
}

bool VistleConnection::waitForReply(const vistle::message::Message &send, vistle::message::Message &reply) const
{
    if (!requestReplyAsync(send)) {
        return false;
    }
    return waitForReplyAsync(send.uuid(), reply);
}

std::vector<std::string> vistle::VistleConnection::getParameters(int id) const
{
    mutex_lock lock(m_mutex);
    return ui().state().getParameters(id);
}

bool vistle::VistleConnection::barrier() const
{
    message::Buffer buf;
    message::Barrier m;
    for (;;) {
        if (!waitForReply(m, buf)) {
            return false;
        }

        switch (buf.type()) {
        case message::BARRIERREACHED: {
            auto &reached = buf.as<message::BarrierReached>();
            if (m.uuid() != reached.uuid()) {
                std::cerr << "VistleConnection: BarrierReached's uuid does not match Barrier's" << std::endl;
            }
            assert(m.uuid() == reached.uuid());
            return true;
            break;
        }
        case message::BARRIER: {
            continue;
            break;
        }
        default:
            std::cerr << "VistleConnection: expected BarrierReached, got " << buf << std::endl;
            assert("expected BarrierReached message" == 0);
            break;
        }
    }

    return false;
}

bool vistle::VistleConnection::resetDataFlowNetwork() const
{
    {
        mutex_lock lock(m_mutex);
        for (int id: ui().state().getRunningList()) {
            message::Kill m(id);
            m.setDestId(id);
            if (!sendMessage(m))
                return false;
        }
    }
    return true;
#if 0
   int barrierId = barrier();
   if (barrierId < 0) {
      std::cerr << "VistleConnection::resetDataFlowNetwork: barrier failed" << std::endl;
      return;
   }
   sendMessage(message::ResetModuleIds());
#endif
}

bool VistleConnection::executeSources() const
{
    mutex_lock lock(m_mutex);
    message::Execute exec;
    exec.setDestId(message::Id::MasterHub);
    return sendMessage(exec);
}

bool VistleConnection::connect(const Port *from, const Port *to) const
{
    message::Connect conn(from->getModuleID(), from->getName(), to->getModuleID(), to->getName());
    return sendMessage(conn);
}

bool VistleConnection::disconnect(const Port *from, const Port *to) const
{
    message::Disconnect disc(from->getModuleID(), from->getName(), to->getModuleID(), to->getName());
    return sendMessage(disc);
}

UiPythonStateAccessor::UiPythonStateAccessor(vistle::VistleConnection *vc): m_vc(vc)
{
    assert(m_vc);
}

void UiPythonStateAccessor::lock()
{
    m_vc->lock();
}

void UiPythonStateAccessor::unlock()
{
    m_vc->unlock();
}

vistle::StateTracker &UiPythonStateAccessor::state()
{
    return m_vc->ui().state();
}

bool UiPythonStateAccessor::sendMessage(const vistle::message::Message &m, const vistle::buffer *payload)
{
    return m_vc->sendMessage(m, payload);
}

} //namespace vistle
