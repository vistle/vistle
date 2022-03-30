#ifndef VISTLECONNECTION_H
#define VISTLECONNECTION_H

#include <string>
#include <mutex>
#include "userinterface.h"
#include <vistle/python/pythonmodule.h>

namespace vistle {

class V_UIEXPORT VistleConnection {
public:
    VistleConnection(vistle::UserInterface &ui);
    virtual ~VistleConnection();
    static VistleConnection &the();

    bool done() const;
    bool started() const;
    void cancel();
    void operator()();

    void setQuitOnExit(bool quit);

    bool sendMessage(const vistle::message::Message &msg, const buffer *payload = nullptr) const;
    bool requestReplyAsync(const vistle::message::Message &send) const;
    bool waitForReplyAsync(const vistle::message::uuid_t &uuid, vistle::message::Message &reply) const;
    bool waitForReply(const vistle::message::Message &send, vistle::message::Message &reply) const;

    std::vector<std::string> getParameters(int id) const;
    std::shared_ptr<vistle::Parameter> getParameter(int id, const std::string &name) const;
    template<class T>
    void setParameter(int id, const std::string &name, const T &value) const;
    bool sendParameter(const std::shared_ptr<Parameter> p) const;

    bool connect(const Port *from, const Port *to) const;
    bool disconnect(const Port *from, const Port *to) const;

    bool barrier() const;
    bool resetDataFlowNetwork() const;
    bool executeSources() const;

    vistle::UserInterface &ui() const;

    void lock();
    void unlock();

private:
    vistle::UserInterface &m_ui;
    bool m_started = false;
    bool m_done = false;
    bool m_quitOnExit;

    typedef std::recursive_mutex mutex;
    typedef std::unique_lock<mutex> mutex_lock;
    mutable mutex m_mutex;

private:
    static VistleConnection *s_instance;
};

template<class T>
void VistleConnection::setParameter(int id, const std::string &name, const T &value) const
{
    mutex_lock lock(m_mutex);
    auto generic = getParameter(id, name);
    if (!generic) {
        std::cerr << "VistleConnection:setParameter: no such parameter: " << id << ":" << name << std::endl;
        return;
    }
    auto p = std::dynamic_pointer_cast<vistle::ParameterBase<T>>(generic);
    if (!p) {
        std::cerr << "VistleConnection:setParameter: parameter not of appropriate type: " << id << ":" << name
                  << std::endl;
        return;
    }

    const T oldval = p->getValue();
    if (oldval != value || p->isDefault()) {
        p->setValue(value);
        sendParameter(p);
    }
}

struct V_UIEXPORT UiPythonStateAccessor: public vistle::PythonStateAccessor {
    UiPythonStateAccessor(vistle::VistleConnection *vc);

    void lock() override;
    void unlock() override;
    StateTracker &state() override;
    bool sendMessage(const vistle::message::Message &m, const buffer *payload = nullptr) override;

    vistle::VistleConnection *m_vc = nullptr;
};

} //namespace vistle
#endif
