#define ENUMS_FOR_PYTHON

#include <cstdio>
#include <thread>
#include <fstream>

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/eval.h>
#include <vistle/util/pybind.h>

#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>

#include <vistle/core/uuid.h>
#include <vistle/core/message.h>
#include <vistle/core/parameter.h>
#include <vistle/core/port.h>

#include "pythonmodule.h"
#ifdef EMBED_PYTHON
#include "pythoninterface.h"
#endif

//#define DEBUG
//#define OBSERVER_DEBUG

#include <vistle/core/statetracker.h>
#include <vistle/core/porttracker.h>

namespace py = pybind11;

#ifdef EMBED_PYTHON
#define PY_MODULE(mod, m) PYBIND11_EMBEDDED_MODULE(mod, m)
#else
#define PY_MODULE(mod, m) PYBIND11_MODULE(mod, m)
#endif


#ifdef VISTLE_CONTROL

// if embedded in Vistle hub
#include <vistle/control/hub.h>
#define MODULEMANAGER (Hub::the().stateTracker())
#define LOCKED() StateTracker::mutex_locker locker(Hub::the().stateTracker().getMutex())

#else

// if part of a user interface
#include "vistleconnection.h"
#define MODULEMANAGER ((pythonModuleInstance->vistleConnection().ui().state()))
#define LOCKED() \
    std::unique_ptr<vistle::VistleConnection::Locker> lock = pythonModuleInstance->vistleConnection().locked()

#endif

#define PORTMANAGER (*MODULEMANAGER.portTracker())

#ifndef EMBED_PYTHON
static std::unique_ptr<vistle::VistleConnection> connection;
static std::unique_ptr<vistle::UserInterface> userinterface;
static std::unique_ptr<vistle::PythonModule> pymod;
static std::unique_ptr<std::thread, std::function<void(std::thread *)>> vistleThread(nullptr, [](std::thread *thr) {
    if (connection) {
        connection->cancel();
    }
    if (thr->joinable())
        thr->join();
    delete thr;
});
#endif

namespace asio = boost::asio;

namespace vistle {

namespace {
PythonModule *pythonModuleInstance = nullptr;
message::Type traceMessages = message::INVALID;
} // namespace

static bool sendMessage(const vistle::message::Message &m, const buffer *payload = nullptr)
{
#ifdef VISTLE_CONTROL
    bool ret = Hub::the().handleMessage(m, nullptr, payload);
    assert(ret);
    if (!ret) {
        std::cerr << "Python: failed to send message " << m << std::endl;
    }
    return ret;
#else
    if (traceMessages == m.type() || traceMessages == message::ANY) {
        std::cerr << "Python: send " << m << std::endl;
    }
    if (!pythonModuleInstance) {
        std::cerr << "cannot send message: no connection to Vistle session" << std::endl;
        return false;
    }
    return pythonModuleInstance->vistleConnection().sendMessage(m, payload);
#endif
}

template<class Payload>
static bool sendMessage(vistle::message::Message &m, Payload &payload)
{
    auto pl = addPayload(m, payload);
    return sendMessage(m, &pl);
}

static std::shared_ptr<message::Buffer> waitForReply(const message::uuid_t &uuid)
{
    py::gil_scoped_release release;
    return MODULEMANAGER.waitForReply(uuid);
}

static bool source(const std::string &filename)
{
#ifdef EMBED_PYTHON
    return PythonInterface::the().exec_file(filename);
#else
    bool ok = false;
    try {
        py::object r = py::eval_file<py::eval_statements>(filename.c_str(), py::globals());
        if (r.ptr()) {
            py::print(r);
            py::print("\n");
        }
        ok = true;
    } catch (py::error_already_set &ex) {
        std::cerr << "source: Error: " << ex.what() << std::endl;
        //std::cerr << "Python exec error" << std::endl;
        PyErr_Print();
        PyErr_Clear();
        ok = false;
    } catch (std::exception &ex) {
        std::cerr << "source: Unknown error: " << ex.what() << std::endl;
        ok = false;
    } catch (...) {
        std::cerr << "source: Unknown error" << std::endl;
        ok = false;
    }
    return ok;
#endif
}


static void quit()
{
#ifdef DEBUG
    std::cerr << "Python: quit" << std::endl;
#endif
    message::Quit m;
    sendMessage(m);
    exit(0);
}

static void ping(int dest = message::Id::Broadcast, char c = '.')
{
#ifdef DEBUG
    std::cerr << "Python: ping: " << c << std::endl;
#endif
    message::Ping m(c);
    m.setDestId(dest);
    sendMessage(m);
}

static void trace(int id = message::Id::Broadcast, message::Type type = message::ANY, bool onoff = true)
{
#ifdef DEBUG
    auto cerrflags = std::cerr.flags();
    std::cerr << "Python: trace " << id < < < < ", type " << type << ": " << std::boolalpha << onoff << std::endl;
    std::cerr.flags(cerrflags);
#endif
    if (id == message::Id::Broadcast || id == message::Id::UI) {
        if (onoff)
            traceMessages = type;
        else
            traceMessages = message::INVALID;
    }

    message::Trace m(id, type, onoff);
    sendMessage(m);
}

static bool barrier()
{
    message::Barrier m;
    m.setDestId(message::Id::MasterHub);
    MODULEMANAGER.registerRequest(m.uuid());
    if (!sendMessage(m))
        return false;
    auto buf = waitForReply(m.uuid());
    if (buf->type() == message::BARRIERREACHED) {
        return true;
    }
    return false;
}

static void removeHub(int hub)
{
    if (message::Id::isHub(hub)) {
        message::Quit m(hub);
        m.setDestId(hub);
        sendMessage(m);
    }
}

static std::string spawnAsync(int hub, const char *module, int numSpawn = -1, int baseRank = -1, int rankSkip = -1)
{
#ifdef DEBUG
    std::cerr << "Python: spawnAsync " << module << std::endl;
#endif

    message::Spawn m(hub, module, numSpawn, baseRank, rankSkip);
    m.setDestId(message::Id::MasterHub); // to master for module id generation
    MODULEMANAGER.registerRequest(m.uuid());
    if (!sendMessage(m))
        return "";
    std::string uuid = boost::lexical_cast<std::string>(m.uuid());

    return uuid;
}

static std::string spawnAsyncSimple(const char *module, int numSpawn = -1, int baseRank = -1, int rankSkip = -1)
{
    return spawnAsync(message::Id::MasterHub, module, numSpawn, baseRank, rankSkip);
}

static int waitForSpawn(const std::string &uuid)
{
    boost::uuids::string_generator gen;
    message::uuid_t u = gen(uuid);
    auto buf = waitForReply(u);
    if (buf->type() == message::SPAWN) {
        auto &spawn = buf->as<message::Spawn>();
        return spawn.spawnId();
    } else {
        std::cerr << "waitForSpawn: got " << buf << std::endl;
    }

    return message::Id::Invalid;
}

static int spawn(int hub, const char *module, int numSpawn = -1, int baseRank = -1, int rankSkip = -1)
{
#ifdef DEBUG
    std::cerr << "Python: spawn " << module << std::endl;
#endif
    const std::string uuid = spawnAsync(hub, module, numSpawn, baseRank, rankSkip);
    return waitForSpawn(uuid);
}

static int spawnSimple(const char *module)
{
    return spawn(message::Id::MasterHub, module);
}

static void kill(int id)
{
#ifdef DEBUG
    std::cerr << "Python: kill " << id << std::endl;
#endif
    if (message::Id::isModule(id)) {
        message::Kill m(id);
        m.setDestId(id);
        sendMessage(m);
    }
}

static int waitForAnySlaveHub()
{
    auto hubs = MODULEMANAGER.getSlaveHubs();
    if (!hubs.empty())
        return hubs[0];

    hubs = MODULEMANAGER.waitForSlaveHubs(1);
    if (!hubs.empty())
        return hubs[0];

    return message::Id::Invalid;
}

static std::vector<int> waitForSlaveHubs(int count)
{
    auto hubs = MODULEMANAGER.waitForSlaveHubs(count);
    return hubs;
}

static int waitForNamedHub(const std::string &name)
{
    std::vector<std::string> names;
    names.push_back(name);
    const auto ids = MODULEMANAGER.waitForSlaveHubs(names);
    for (const auto id: ids) {
        const auto n = MODULEMANAGER.hubName(id);
        if (n == name) {
            return id;
        }
    }
    return message::Id::Invalid;
}

static std::vector<int> waitForNamedHubs(const std::vector<std::string> &names)
{
    return MODULEMANAGER.waitForSlaveHubs(names);
}

static int getHub(int id)
{
    LOCKED();
    return MODULEMANAGER.getHub(id);
}

static int getMasterHub()
{
    LOCKED();
    return MODULEMANAGER.getMasterHub();
}

static int getVistleSession()
{
    return vistle::message::Id::Vistle;
}

static std::vector<int> getAllHubs()
{
    LOCKED();
    return MODULEMANAGER.getSlaveHubs();
}

static std::vector<int> getRunning()
{
    LOCKED();
#ifdef DEBUG
    std::cerr << "Python: getRunning " << std::endl;
#endif
    return MODULEMANAGER.getRunningList();
}

static std::vector<std::string> getAvailable()
{
    LOCKED();
    const auto &avail = MODULEMANAGER.availableModules();
    std::vector<std::string> ret;
    for (auto &a: avail) {
        auto &m = a.second;
        ret.push_back(m.name());
    }
    return ret;
}

static std::vector<int> getBusy()
{
    LOCKED();
#ifdef DEBUG
    std::cerr << "Python: getBusy " << std::endl;
#endif
    return MODULEMANAGER.getBusyList();
}

static std::vector<std::string> getInputPorts(int id)
{
    LOCKED();
    return PORTMANAGER.getInputPortNames(id);
}

static std::vector<std::string> getOutputPorts(int id)
{
    LOCKED();
    return PORTMANAGER.getOutputPortNames(id);
}

std::string getPortDescription(int id, const std::string &portname)
{
    LOCKED();
    return PORTMANAGER.getPortDescription(id, portname);
}


static std::vector<std::pair<int, std::string>> getConnections(int id, const std::string &port)
{
    LOCKED();
    std::vector<std::pair<int, std::string>> result;

    if (const Port::ConstPortSet *c = PORTMANAGER.getConnectionList(id, port)) {
        for (const Port *p: *c) {
            result.push_back(std::pair<int, std::string>(p->getModuleID(), p->getName()));
        }
    }

    return result;
}

static std::vector<std::string> getParameters(int id)
{
    LOCKED();
    return MODULEMANAGER.getParameters(id);
}

static std::string getParameterType(int id, const std::string &name)
{
    LOCKED();
    const auto param = MODULEMANAGER.getParameter(id, name);
    if (!param) {
        std::cerr << "Python: getParameterType: no such parameter" << std::endl;
        return "None";
    }
    switch (param->type()) {
    case Parameter::Integer:
        return "Int"; //shorter to type for the most common type
    case Parameter::Invalid:
    case Parameter::Unknown:
        return "None";
    default:
        return vistle::Parameter::toString(param->type());
    }
}

static std::string getParameterPresentation(int id, const std::string &name)
{
    LOCKED();
    const auto param = MODULEMANAGER.getParameter(id, name);
    if (!param) {
        std::cerr << "Python: getParameterPresentation: no such parameter" << std::endl;
        return "None";
    }
    return vistle::Parameter::toString(param->presentation());
}


static bool isParameterDefault(int id, const std::string &name)
{
    LOCKED();
    const auto param = MODULEMANAGER.getParameter(id, name);
    if (!param) {
        std::cerr << "Python: isParameterDefault: no such parameter: id=" << id << ", name=" << name << std::endl;
        return false;
    }

    return param->isDefault();
}

template<typename T>
static T getParameterValue(int id, const std::string &name)
{
    LOCKED();
    const auto param = MODULEMANAGER.getParameter(id, name);
    if (!param) {
        std::cerr << "Python: getParameterValue: no such parameter: id=" << id << ", name=" << name << std::endl;
        return T();
    }

    const auto tparam = std::dynamic_pointer_cast<const ParameterBase<T>>(param);
    if (!tparam) {
        std::cerr << "Python: getParameterValue: type mismatch: id=" << id << ", name=" << name << std::endl;
        return T();
    }

    return tparam->getValue();
}

static std::string getParameterTooltip(int id, const std::string &name)
{
    LOCKED();
    const auto param = MODULEMANAGER.getParameter(id, name);
    if (!param) {
        std::cerr << "Python: getParameterTooltip: no such parameter" << std::endl;
        return "None";
    }
    auto desc = param->description();
    if (param->presentation() == Parameter::Choice) {
        desc += " (";
        for (const auto &c: param->choices())
            desc += c + ", ";
        desc.pop_back();
        desc.pop_back();
        desc += ")";
    }
    return desc;
}

static std::string getEscapedStringParam(int id, const std::string &name)
{
    std::string val = getParameterValue<std::string>(id, name);
    std::vector<char> escaped;

    for (auto c: val) {
        switch (c) {
        case '\"':
            escaped.emplace_back('\\');
            escaped.emplace_back('\"');
            break;
        case '\'':
            escaped.emplace_back('\\');
            escaped.emplace_back('\'');
            break;
        case '\\':
            escaped.emplace_back('\\');
            escaped.emplace_back('\\');
            break;
        case '\a':
            escaped.emplace_back('\\');
            escaped.emplace_back('a');
            break;
        case '\b':
            escaped.emplace_back('\\');
            escaped.emplace_back('b');
            break;
        case '\n':
            escaped.emplace_back('\\');
            escaped.emplace_back('n');
            break;
        case '\r':
            escaped.emplace_back('\\');
            escaped.emplace_back('r');
            break;
        case '\t':
            escaped.emplace_back('\\');
            escaped.emplace_back('t');
            break;
        default:
            if (iscntrl(c)) {
                escaped.emplace_back('\\');
                escaped.emplace_back('0' + c / 8 / 8);
                escaped.emplace_back('0' + (c / 8) % 8);
                escaped.emplace_back('0' + c % 8);
            } else {
                escaped.emplace_back(c);
            }
            break;
        }
    }

    return std::string(escaped.data(), escaped.size());
}

static std::string getModuleName(int id)
{
    LOCKED();
#ifdef DEBUG
    std::cerr << "Python: getModuleName(" id << ")" << std::endl;
#endif
    return MODULEMANAGER.getModuleName(id);
}

static std::string getModuleDescription(int id)
{
    LOCKED();
#ifdef DEBUG
    std::cerr << "Python: getModuleDescription(" id << ")" << std::endl;
#endif
    return MODULEMANAGER.getModuleDescription(id);
}

static void connect(int sid, const char *sport, int did, const char *dport)
{
#ifdef DEBUG
    std::cerr << "Python: connect " << sid << ":" << sport << " -> " << did << ":" << dport << std::endl;
#endif
    message::Connect m(sid, sport, did, dport);
    sendMessage(m);
}

static void disconnect(int sid, const char *sport, int did, const char *dport)
{
#ifdef DEBUG
    std::cerr << "Python: disconnect " << sid << ":" << sport << " -> " << did << ":" << dport << std::endl;
#endif
    message::Disconnect m(sid, sport, did, dport);
    sendMessage(m);
}

static void setIntParam(int id, const char *name, Integer value, bool delayed)
{
#ifdef DEBUG
    std::cerr << "Python: setIntParam " << id << ":" << name << " = " << value << std::endl;
#endif
    message::SetParameter m(id, name, value);
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void setFloatParam(int id, const char *name, Float value, bool delayed)
{
#ifdef DEBUG
    std::cerr << "Python: setFloatParam " << id << ":" << name << " = " << value << std::endl;
#endif
    message::SetParameter m(id, name, value);
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void setVectorParam4(int id, const char *name, Float v1, Float v2, Float v3, Float v4, bool delayed)
{
    message::SetParameter m(id, name, ParamVector(v1, v2, v3, v4));
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void setVectorParam3(int id, const char *name, Float v1, Float v2, Float v3, bool delayed)
{
    message::SetParameter m(id, name, ParamVector(v1, v2, v3));
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void setVectorParam2(int id, const char *name, Float v1, Float v2, bool delayed)
{
    message::SetParameter m(id, name, ParamVector(v1, v2));
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void setVectorParam1(int id, const char *name, Float v1, bool delayed)
{
    message::SetParameter m(id, name, ParamVector(v1));
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void setIntVectorParam4(int id, const char *name, Integer v1, Integer v2, Integer v3, Integer v4, bool delayed)
{
    message::SetParameter m(id, name, IntParamVector(v1, v2, v3, v4));
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void setIntVectorParam3(int id, const char *name, Integer v1, Integer v2, Integer v3, bool delayed)
{
    message::SetParameter m(id, name, IntParamVector(v1, v2, v3));
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void setIntVectorParam2(int id, const char *name, Integer v1, Integer v2, bool delayed)
{
    message::SetParameter m(id, name, IntParamVector(v1, v2));
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void setIntVectorParam1(int id, const char *name, Integer v1, bool delayed)
{
    message::SetParameter m(id, name, IntParamVector(v1));
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void setStringParam(int id, const char *name, const std::string &value, bool delayed)
{
#ifdef DEBUG
    std::cerr << "Python: setStringParam " << id << ":" << name << " = " << value << std::endl;
#endif
    message::SetParameter m(id, name, value);
    m.setDestId(id);
    if (delayed)
        m.setDelayed();
    sendMessage(m);
}

static void applyParameters(int id)
{
#ifdef DEBUG
    std::cerr << "Python: applyParameters " << id << std::endl;
#endif
    message::SetParameter m(id);
    m.setDestId(id);
    sendMessage(m);
}

static void compute(int id = message::Id::Broadcast)
{
#ifdef DEBUG
    std::cerr << "Python: compute " << id << std::endl;
#endif
    message::Execute m(message::Execute::ComputeExecute, id);
    if (id == message::Id::Broadcast)
        m.setDestId(message::Id::MasterHub);
    else
        m.setDestId(id);
    sendMessage(m);
}

static void cancelCompute(int id)
{
#ifdef DEBUG
    std::cerr << "Python: cancelCompute " << id << std::endl;
#endif
    message::CancelExecute m(id);
    m.setDestId(id);
    sendMessage(m);
}

static void requestTunnel(unsigned short listenPort, const std::string &destHost, unsigned short destPort = 0)
{
#ifdef DEBUG
    std::cerr << "Python: requestTunnel " << listenPort << " -> " << destHost << ":" << destPort << std::endl;
#endif
    if (destPort == 0)
        destPort = listenPort;

    message::RequestTunnel m(listenPort, destHost, destPort);

    asio::io_service io_service;
    asio::ip::tcp::resolver resolver(io_service);
    try {
        auto endpoints = resolver.resolve({destHost, boost::lexical_cast<std::string>(destPort)});
        auto addr = (*endpoints).endpoint().address();
        if (addr.is_v6()) {
            m.setDestAddr(addr.to_v6());
            std::cerr << destHost << " resolved to " << addr.to_v6() << std::endl;
        } else if (addr.is_v4()) {
            m.setDestAddr(addr.to_v4());
            std::cerr << destHost << " resolved to " << addr.to_v4() << std::endl;
        }
    } catch (...) {
    }

    sendMessage(m);
}

static void removeTunnel(unsigned short listenPort)
{
#ifdef DEBUG
    std::cerr << "Python: removeTunnel " << listenPort << std::endl;
#endif

    message::RequestTunnel m(listenPort);
    sendMessage(m);
}

static void printInfo(const std::string &message)
{
#ifdef DEBUG
    std::cerr << "Python: printInfo " << message << std::endl;
#endif

    message::SendText m(message::SendText::Info);
    message::SendText::Payload pl(message);
    sendMessage(m, pl);
}

static void printWarning(const std::string &message)
{
#ifdef DEBUG
    std::cerr << "Python: printWarning " << message << std::endl;
#endif

    message::SendText m(message::SendText::Warning);
    message::SendText::Payload pl(message);
    sendMessage(m, pl);
}

static void printError(const std::string &message)
{
#ifdef DEBUG
    std::cerr << "Python: printError " << message << std::endl;
#endif

    message::SendText m(message::SendText::Error);
    message::SendText::Payload pl(message);
    sendMessage(m, pl);
}

static void setStatus(const std::string &text, message::UpdateStatus::Importance prio)
{
    message::UpdateStatus m(text, prio);
    sendMessage(m);
}

static void clearStatus()
{
    setStatus(std::string(), message::UpdateStatus::Bulk);
}

static void setLoadedFile(const std::string &file)
{
    message::UpdateStatus m(message::UpdateStatus::LoadedFile, file);
    sendMessage(m);
}

static std::string getLoadedFile()
{
    return MODULEMANAGER.loadedWorkflowFile();
}

static std::string getSessionUrl()
{
    return MODULEMANAGER.sessionUrl();
}
//contains allocated compounds
//key is compoundId
//submoduleIds are compoundId + 1 + pos in AvailableModule::submodules()
namespace detail {
struct ModuleCompound {
    int id;
    vistle::ModuleCompound module;
};
} // namespace detail
} // namespace vistle

namespace vistle {
std::vector<detail::ModuleCompound> compounds;

std::vector<detail::ModuleCompound>::iterator findCompound(int id)
{
    return std::find_if(compounds.begin(), compounds.end(),
                        [id](const detail::ModuleCompound &c) { return c.id == id; });
}

static void loadScript(const std::string &filename)
{
#ifdef EMBED_PYTHON
    std::cerr << "loading " << filename << std::endl;
    PythonInterface::the().exec_file(filename);
#endif
}

static int moduleCompoundAlloc(const std::string &compoundName)
{
    auto comp = std::find_if(compounds.begin(), compounds.end(), [&compoundName](const detail::ModuleCompound &c) {
        return c.module.name() == compoundName;
    });
    if (comp == compounds.end()) {
        int i = 0;
        while (true) {
            --i;
            auto it = findCompound(i);
            if (it == compounds.end()) {
                compounds.push_back(detail::ModuleCompound{i, ModuleCompound{0, compoundName, "", ""}});
                return i;
            }
        }
    } else {
        printError("Compound " + compoundName + " already exists!");
        return comp->id;
    }
}

static void compoundNotFoundError(int compoundId)
{
    printError("module compound with id " + std::to_string(compoundId) + "hast not been found!");
    printError("allocate it with moduleCompoundAlloc(\"compoundName\"");
}

static int moduleCompoundAddModule(int compoundId, const std::string &moduleName, float x = 0, float y = 0)
{
    auto comp = findCompound(compoundId);
    if (comp == compounds.end()) {
        compoundNotFoundError(compoundId);
        return 0;
    }
    return comp->module.addSubmodule(ModuleCompound::SubModule{moduleName, x, y});
}

static void moduleCompoundConnect(int compoundId, int fromId, const std::string &fromName, int toId,
                                  const std::string &toName)
{
    auto comp = findCompound(compoundId);
    if (comp == compounds.end()) {
        compoundNotFoundError(compoundId);
        return;
    }
    if (fromId != compoundId && //not to expose
        fromId - compoundId >= static_cast<int>(comp->module.submodules().size()) && //module not added
        fromId < 1) //invalid
    {
        printError("moduleCompoundConnect: invalid fromId " + std::to_string(fromId));
        return;
    }
    if (toId != compoundId && //not to expose
        toId - compoundId >= static_cast<int>(comp->module.submodules().size()) && //module not added
        toId < 1) //invalid
    {
        printError("moduleCompoundConnect: invalid toId " + std::to_string(fromId));
        return;
    }

    comp->module.addConnection(
        ModuleCompound::Connection{fromId - compoundId - 1, toId - compoundId - 1, fromName, toName});
}


Float compoundDropPositionX = 0;
Float compoundDropPositionY = 0;

static void setCompoundDropPosition(Float x, Float y)
{
    compoundDropPositionX = x;
    compoundDropPositionY = y;
}

static void setRelativePos(int id, Float x, Float y)
{
    setVectorParam2(id, "_position", x + compoundDropPositionX, y + compoundDropPositionY, true);
}

void spawnAvailablModule(const AvailableModule &comp)
{
    std::cerr << "writing file " << comp.name() << moduleCompoundSuffix << std::endl;
    auto filename = comp.path().empty() ? comp.name() : comp.path();
    if (filename.find_last_of(moduleCompoundSuffix) != filename.size() - 1)
        filename += moduleCompoundSuffix;

    std::fstream file{filename, std::ios_base::out};
    file << "MasterHub=getMasterHub()" << std::endl;
    for (size_t i = 0; i < comp.submodules().size(); i++) {
        file << "um" << comp.submodules()[i].name << i << " = spawnAsync(MasterHub, '" << comp.submodules()[i].name
             << "')" << std::endl;
    }
    file << std::endl;
    for (size_t i = 0; i < comp.submodules().size(); i++) {
        float posX = comp.submodules()[i].x - comp.submodules()[0].x;
        float posY = comp.submodules()[i].y - comp.submodules()[0].y;
        file << "m" << comp.submodules()[i].name << i << " = waitForSpawn(um" << comp.submodules()[i].name << i << ")"
             << std::endl;
        file << "setRelativePos("
             << "m" << comp.submodules()[i].name << i << ", " << posX << ", " << posY << ")" << std::endl;
        file << "applyParameters(m" << comp.submodules()[i].name << i << ")" << std::endl;
    }
    for (const auto &conn: comp.connections()) {
        if (conn.fromId >= 0 && conn.toId >= 0) //internal connection
        {
            file << "connect(m" << comp.submodules()[conn.fromId].name << conn.fromId << ",'" << conn.fromPort << "', m"
                 << comp.submodules()[conn.toId].name << conn.toId << ",'" << conn.toPort << "')" << std::endl;
        }
    }
}

void moduleCompoundToFile(const ModuleCompound &comp)
{
    std::cerr << "writing file " << comp.name() << moduleCompoundSuffix << std::endl;
    auto filename = comp.path().empty() ? comp.name() : comp.path();
    if (filename.find_last_of(moduleCompoundSuffix) != filename.size() - 1)
        filename += moduleCompoundSuffix;

    std::fstream file{filename, std::ios_base::out};
    file << "CompoundId=moduleCompoundAlloc(\"" << comp.name() << "\")" << std::endl;
    for (size_t i = 0; i < comp.submodules().size(); i++) {
        file << "um" << comp.submodules()[i].name << i << " = moduleCompoundAddModule(CompoundId, \""
             << comp.submodules()[i].name << "\", " << comp.submodules()[i].x << ", " << comp.submodules()[i].y << ")"
             << std::endl;
    }
    file << std::endl;
    for (const auto &conn: comp.connections()) {
        file << "moduleCompoundConnect(CompoundId, " << conn.fromId << ",\"" << conn.fromPort << "\", " << conn.toId
             << ", \"" << conn.toPort << "\")" << std::endl;
    }
    file << "moduleCompoundCreate(CompoundId)" << std::endl;
}

static void moduleCompoundCreate(int compoundId)
{
    auto mc = findCompound(compoundId);
    if (mc == compounds.end()) {
        printError("Failed to create module compound: invalid id " + std::to_string(compoundId));
        printError("Module compound has to be allocated using \"moduleCompoundAlloc(string compoundName)\"");
        return;
    }
    mc->module.send(
        [](const message::Message &msg, const buffer *payload) -> bool { return sendMessage(msg, payload); });
    compounds.erase(mc);
}

class TrivialStateObserver: public StateObserver {
public:
    TrivialStateObserver()
#ifdef OBSERVER_DEBUG
    : m_out(std::cerr)
#endif
    {}

    void newHub(int hub, const std::string &name, int nranks, const std::string &address, const std::string &logname,
                const std::string &realname) override
    {}
    void deleteHub(int hub) override {}
    void moduleAvailable(const AvailableModule &mod) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   hub: " << mod.hub() << ", module: " << mod.name() << " (" << mod.path() << ")" << std::endl;
#endif
    }

    void newModule(int moduleId, const boost::uuids::uuid &spawnUuid, const std::string &moduleName) override
    {
        (void)spawnUuid;
#ifdef OBSERVER_DEBUG
        m_out << "   module " << moduleName << " started: " << moduleId << std::endl;
#endif
    }

    void deleteModule(int moduleId) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   module deleted: " << moduleId << std::endl;
#endif
    }

    void moduleStateChanged(int moduleId, int stateBits) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   module state change: " << moduleId << " (";
        if (stateBits & StateObserver::Initialized)
            m_out << "I";
        if (stateBits & StateObserver::Killed)
            m_out << "K";
        if (stateBits & StateObserver::Busy)
            m_out << "B";
        m_out << ")" << std::endl;
#endif
    }

    void newParameter(int moduleId, const std::string &parameterName) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   new parameter: " << moduleId << ":" << parameterName << std::endl;
#endif
    }

    void deleteParameter(int moduleId, const std::string &parameterName) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   delete parameter: " << moduleId << ":" << parameterName << std::endl;
#endif
    }

    void parameterValueChanged(int moduleId, const std::string &parameterName) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   parameter value changed: " << moduleId << ":" << parameterName << std::endl;
#endif
    }

    void parameterChoicesChanged(int moduleId, const std::string &parameterName) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   parameter choices changed: " << moduleId << ":" << parameterName << std::endl;
#endif
    }

    void newPort(int moduleId, const std::string &portName) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   new port: " << moduleId << ":" << portName << std::endl;
#endif
    }

    void deletePort(int moduleId, const std::string &portName) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   delete port: " << moduleId << ":" << portName << std::endl;
#endif
    }

    void newConnection(int fromId, const std::string &fromName, int toId, const std::string &toName) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   new connection: " << fromId << ":" << fromName << " -> " << toId << ":" << toName << std::endl;
#endif
    }

    void deleteConnection(int fromId, const std::string &fromName, int toId, const std::string &toName) override
    {
#ifdef OBSERVER_DEBUG
        m_out << "   connection removed: " << fromId << ":" << fromName << " -> " << toId << ":" << toName << std::endl;
#endif
    }

    void info(const std::string &text, message::SendText::TextType textType, int senderId, int senderRank,
              message::Type refType, const message::uuid_t &refUuid) override
    {
#ifdef OBSERVER_DEBUG
        std::cerr << senderId << "(" << senderRank << "): " << text << std::endl;
#endif
    }

    void status(int id, const std::string &text, message::UpdateStatus::Importance prio) override
    {
#ifdef OBSERVER_DEBUG
        std::cerr << "Module status: " << id << ": " << text << std::endl;
#endif
    }

    void updateStatus(int id, const std::string &text, message::UpdateStatus::Importance prio) override
    {
#ifdef OBSERVER_DEBUG
        std::cerr << "Overall status: " << id << ": " << text << std::endl;
#endif
    }

#ifdef OBSERVER_DEBUG
private:
    std::ostream &m_out;
#endif
};


class PyStateObserver: public TrivialStateObserver {
    typedef TrivialStateObserver Base;

public:
    using TrivialStateObserver::TrivialStateObserver;

    void moduleAvailable(const AvailableModule &mod) override
    {
        PYBIND11_OVERLOAD(void, /* Return type */
                          Base, /* Parent class */
                          moduleAvailable, /* Name of function in C++ (must match Python name) */
                          mod /* parameters */
        );
    }

    void newHub(int hub, const std::string &name, int nranks, const std::string &address, const std::string &logname,
                const std::string &realname) override
    {
        PYBIND11_OVERLOAD(void, Base, newHub, hub, name, nranks, address, logname, realname);
    }

    void deleteHub(int hub) override { PYBIND11_OVERLOAD(void, Base, deleteHub, hub); }

    void newModule(int moduleId, const boost::uuids::uuid &spawnUuid, const std::string &moduleName) override
    {
        PYBIND11_OVERLOAD(void, Base, newModule, moduleId, spawnUuid, moduleName);
    }

    void deleteModule(int moduleId) override { PYBIND11_OVERLOAD(void, Base, deleteModule, moduleId); }

    void moduleStateChanged(int moduleId, int stateBits) override
    {
        PYBIND11_OVERLOAD(void, Base, moduleStateChanged, moduleId, stateBits);
    }

    void newParameter(int moduleId, const std::string &parameterName) override
    {
        PYBIND11_OVERLOAD(void, Base, newParameter, moduleId, parameterName);
    }

    void deleteParameter(int moduleId, const std::string &parameterName) override
    {
        PYBIND11_OVERLOAD(void, Base, deleteParameter, moduleId, parameterName);
    }

    void parameterValueChanged(int moduleId, const std::string &parameterName) override
    {
        PYBIND11_OVERLOAD(void, Base, parameterValueChanged, moduleId, parameterName);
    }

    void parameterChoicesChanged(int moduleId, const std::string &parameterName) override
    {
        PYBIND11_OVERLOAD(void, Base, parameterChoicesChanged, moduleId, parameterName);
    }

    void newPort(int moduleId, const std::string &portName) override
    {
        PYBIND11_OVERLOAD(void, Base, newPort, moduleId, portName);
    }

    void deletePort(int moduleId, const std::string &portName) override
    {
        PYBIND11_OVERLOAD(void, Base, deletePort, moduleId, portName);
    }

    void newConnection(int fromId, const std::string &fromName, int toId, const std::string &toName) override
    {
        PYBIND11_OVERLOAD(void, Base, newConnection, fromId, fromName, toId, toName);
    }

    void deleteConnection(int fromId, const std::string &fromName, int toId, const std::string &toName) override
    {
        PYBIND11_OVERLOAD(void, Base, deleteConnection, fromId, fromName, toId, toName);
    }

    void info(const std::string &text, message::SendText::TextType textType, int senderId, int senderRank,
              message::Type refType, const message::uuid_t &refUuid) override
    {
        PYBIND11_OVERLOAD(void, Base, info, text, textType, senderId, senderRank, refType, refUuid);
    }

    void status(int id, const std::string &text, message::UpdateStatus::Importance prio) override
    {
        PYBIND11_OVERLOAD(void, Base, status, id, text, prio);
    }

    void updateStatus(int id, const std::string &text, message::UpdateStatus::Importance prio) override
    {
        PYBIND11_OVERLOAD(void, Base, updateStatus, id, text, prio);
    }
};

#ifndef EMBED_PYTHON
static bool sessionConnectWithObserver(StateObserver *o, const std::string &host, unsigned short port)
{
    if (userinterface || connection || pymod || vistleThread) {
        std::cerr << "already connected" << std::endl;
        return false;
    }

    userinterface.reset(new UserInterface(host, port, o));
    if (!userinterface)
        return false;
    connection.reset(new VistleConnection(*userinterface));
    if (!connection)
        return false;
    pymod.reset(new PythonModule(connection.get()));
    if (!pymod)
        return false;
    vistleThread.reset(new std::thread(std::ref(*connection)));
    if (!vistleThread)
        return false;

    while (!userinterface->isInitialized()) {
        usleep(100);
    }

    return true;
}

static bool sessionConnect(const std::string &host, unsigned short port)
{
    return sessionConnectWithObserver(nullptr, host, port);
}

static bool sessionDisconnect()
{
    if (!vistleThread)
        return false;
    if (!pymod)
        return false;
    if (!connection)
        return false;
    if (!userinterface)
        return false;

    vistleThread.reset();
    pymod.reset();
    connection.reset();
    userinterface.reset();

    return true;
}
#endif


#define param1(T, f) \
    m.def("set" #T "Param", &f, "set parameter `name` of module with `id` to `value`", "id"_a, "name"_a, "value"_a, \
          "delayed"_a = false); \
    m.def("setParam", &f, "set parameter `name` of module with `id` to `value`", "id"_a, "name"_a, "value"_a, \
          "delayed"_a = false);

#define param2(T, f) \
    m.def("set" #T "Param", &f, "set parameter `name` of module with `id` to (`value1`, `value2`)", "id"_a, "name"_a, \
          "value1"_a, "value2"_a, "delayed"_a = false); \
    m.def("setParam", &f, "set parameter `name` of module with `id` to `(value1, value2)`", "id"_a, "name"_a, \
          "value1"_a, "value2"_a, "delayed"_a = false);

#define param3(T, f) \
    m.def("set" #T "Param", &f, "set parameter `name` of module with `id` to (`value1`, `value2`, `value3`)", "id"_a, \
          "name"_a, "value1"_a, "value2"_a, "value3"_a, "delayed"_a = false); \
    m.def("setParam", &f, "set parameter `name` of module with `id` to `(value1, value2, `value3`)", "id"_a, "name"_a, \
          "value1"_a, "value2"_a, "value3"_a, "delayed"_a = false);

#define param4(T, f) \
    m.def("set" #T "Param", &f, \
          "set parameter `name` of module with `id` to (`value1`, `value2`, `value3`, `value4`)", "id"_a, "name"_a, \
          "value1"_a, "value2"_a, "value3"_a, "value4"_a, "delayed"_a = false); \
    m.def("setParam", &f, "set parameter `name` of module with `id` to `(value1, value2, `value3`, `value4`)", "id"_a, \
          "name"_a, "value1"_a, "value2"_a, "value3"_a, "value4"_a, "delayed"_a = false);

PY_MODULE(_vistle, m)
{
    using namespace py::literals;
    m.doc() = "Vistle Python bindings";

    py::class_<boost::uuids::uuid> uuidt(m, "uuid");
    uuidt.def(py::init<>());
    uuidt.def("__repr__", [](const boost::uuids::uuid &id) { return boost::lexical_cast<std::string>(id); });

    // make values of vistle::message::Type enum known to Python as Message.xxx
    py::class_<message::Message> message(m, "Message");
    vistle::message::enumForPython_Type(message, "Message");

    // make values of vistle::message::UpdateStatus::Importance enum known to Python as Importance.xxx
    py::class_<message::UpdateStatus> us(m, "Status");
    vistle::message::UpdateStatus::enumForPython_Importance(us, "Importance");

    // make values of vistle::message::SendText::TextType enum known to Python as Text.xxx
    py::class_<message::SendText> st(m, "Text");
    vistle::message::SendText::enumForPython_TextType(st, "Type");

    py::class_<message::Id> id(m, "Id");
    py::enum_<message::Id::Reserved>(id, "Id")
        .value("Invalid", message::Id::Invalid)
        .value("Vistle", message::Id::Vistle)
        .value("Broadcast", message::Id::Broadcast)
        .value("ForBroadcast", message::Id::ForBroadcast)
        .value("NextHop", message::Id::NextHop)
        .value("UI", message::Id::UI)
        .value("LocalManager", message::Id::LocalManager)
        .value("LocalHub", message::Id::LocalHub)
        .value("MasterHub", message::Id::MasterHub)
        .export_values();

    typedef vistle::StateObserver SO;
    py::class_<StateObserver>(m, "StateObserverBase")
        //.def(py::init<>())
        .def("moduleAvailable", &SO::moduleAvailable)
        .def("newModule", &SO::newModule)
        .def("deleteModule", &SO::deleteModule)
        .def("moduleStateChanged", &SO::moduleStateChanged)
        .def("newParameter", &SO::newParameter)
        .def("deleteParameter", &SO::deleteParameter)
        .def("parameterValueChanged", &SO::parameterValueChanged)
        .def("parameterChoicesChanged", &SO::parameterChoicesChanged)
        .def("newPort", &SO::newPort)
        .def("deletePort", &SO::deletePort)
        .def("newConnection", &SO::newConnection)
        .def("deleteConnection", &SO::deleteConnection)
        .def("info", &SO::info)
        .def("status", &SO::status)
        .def("updateStatus", &SO::updateStatus);

    typedef vistle::TrivialStateObserver TSO;
    py::class_<TrivialStateObserver, PyStateObserver, StateObserver>(m, "StateObserver")
        .def(py::init([]() { return new PyStateObserver; }))
        //.def(py::init<>())
        .def("moduleAvailable", &TSO::moduleAvailable)
        .def("newModule", &TSO::newModule)
        .def("deleteModule", &TSO::deleteModule)
        .def("moduleStateChanged", &TSO::moduleStateChanged)
        .def("newParameter", &TSO::newParameter)
        .def("deleteParameter", &TSO::deleteParameter)
        .def("parameterValueChanged", &TSO::parameterValueChanged)
        .def("parameterChoicesChanged", &TSO::parameterChoicesChanged)
        .def("newPort", &TSO::newPort)
        .def("deletePort", &TSO::deletePort)
        .def("newConnection", &TSO::newConnection)
        .def("deleteConnection", &TSO::deleteConnection)
        .def("info", &TSO::info)
        .def("status", &TSO::status)
        .def("updateStatus", &TSO::updateStatus);

    m.def("source", &source, "execute commands from `file`", "file"_a);
    m.def("removeHub", &removeHub, "remove hub `id` from session", "id"_a);
    m.def("spawn", spawn,
          "spawn new module `arg1`\n"
          "return its ID",
          "hub"_a, "modulename"_a, "numspawn"_a = -1, "baserank"_a = -1, "rankskip"_a = -1);
    m.def("loadScript", &loadScript, "load a python script", "filename"_a);
    m.def("moduleCompoundAlloc", &moduleCompoundAlloc, "allocate a new module compound", "name"_a);
    m.def("moduleCompoundAddModule", &moduleCompoundAddModule, "add a module to a module compound", "compoundId"_a,
          "modulename"_a, "x"_a = 0, "y"_a = 0);
    m.def("moduleCompoundConnect", &moduleCompoundConnect,
          "connect ports of modules inside the compound, expose port if compound id is given", "compoundId"_a,
          "fromId"_a, "toId"_a, "fromPort"_a, "toPort"_a);
    m.def("setRelativePos", &setRelativePos, "move module relative to compound drop position", "moduleId"_a, "x"_a,
          "y"_a);
    m.def("setCompoundDropPosition", &setCompoundDropPosition, "set the position for a module compound", "x"_a, "y"_a);
    m.def("moduleCompoundCreate", &moduleCompoundCreate, "lock and create the compound", "compoundId"_a);

    m.def("spawn", spawn,
          "spawn new module `arg1`\n"
          "return its ID",
          "hub"_a, "modulename"_a, "numspawn"_a = -1, "baserank"_a = -1, "rankskip"_a = -1);
    m.def("spawn", spawnSimple,
          "spawn new module `arg1`\n"
          "return its ID");
    m.def("spawnAsync", spawnAsync,
          "spawn new module `arg1`\n"
          "return uuid to wait on its ID",
          "hub"_a, "modulename"_a, "numspawn"_a = -1, "baserank"_a = -1, "rankskip"_a = -1);
    m.def("spawnAsync", spawnAsyncSimple,
          "spawn new module `arg1`\n"
          "return uuid to wait on its ID",
          "modulename"_a, "numspawn"_a = 1, "baserank"_a = -1, "rankskip"_a = -1);
    m.def("waitForSpawn", waitForSpawn, "wait for asynchronously spawned module with uuid `arg1` and return its ID");
    m.def("kill", kill, "kill module with ID `arg1`");
    m.def("connect", connect,
          "connect output `arg2` of module with ID `arg1` to input `arg4` of module with ID `arg3`");
    m.def("disconnect", disconnect,
          "disconnect output `arg2` of module with `id1` to input `arg4` of module with `id2`");
    m.def("compute", compute, "trigger execution of module with `id`", "moduleId"_a = message::Id::Broadcast);
    m.def("interrupt", cancelCompute, "interrupt execution of module with ID `arg1`");
    m.def("quit", quit, "quit vistle session");
    m.def("ping", ping, "send first character of `arg2` to destination `arg1`", "id"_a, "data"_a = "p");
    m.def("trace", trace, "enable/disable message tracing for module `id`", "id"_a = message::Id::Broadcast,
          "type"_a = message::ANY, "enable"_a = true);
    m.def("barrier", barrier, "wait until all modules reply");
    m.def("requestTunnel", requestTunnel,
          "start TCP tunnel listening on port `arg1` on hub forwarding incoming connections to `arg2`:`arg3`",
          "listen port"_a, "dest port"_a, "dest addr"_a);
    m.def("removeTunnel", removeTunnel, "remove TCP tunnel listening on port `arg1` on hub");
    //def("checkMessageQueue", checkMessageQueue, "check whether all messages have been processed");
    m.def("printInfo", printInfo, "show info message to user");
    m.def("printWarning", printWarning, "show warning message to user");
    m.def("printError", printError, "show error message to user");
    m.def("setStatus", setStatus, "update status information", "text"_a, "importance"_a = message::UpdateStatus::Low);
    //m.def("setStatus", setStatus, "update status information");
    m.def("clearStatus", clearStatus, "clear status information");
    m.def("setLoadedFile", setLoadedFile, "update name of currently loaded workflow description file");
    m.def("getLoadedFile", getLoadedFile, "name of currently loaded workflow description file");
    m.def("getSessionUrl", getSessionUrl, "URI for connecting to current session");

    param1(Int, setIntParam);
    param1(Float, setFloatParam);
    param1(String, setStringParam);
    param1(Vector, setVectorParam1);
    param2(Vector, setVectorParam2);
    param3(Vector, setVectorParam3);
    param4(Vector, setVectorParam4);
    param1(IntVector, setIntVectorParam1);
    param2(IntVector, setIntVectorParam2);
    param3(IntVector, setIntVectorParam3);
    param4(IntVector, setIntVectorParam4);

    m.def("applyParameters", applyParameters, "apply delayed parameter changes");
    m.def("getAvailable", getAvailable, "get list of names of available modules");
    m.def("getRunning", getRunning, "get list of IDs of running modules");
    m.def("getBusy", getBusy, "get list of IDs of busy modules");
    m.def("getModuleName", getModuleName, "get name of module with ID `arg1`");
    m.def("getModuleDescription", getModuleDescription, "get description of module with ID `arg1`");
    m.def("getInputPorts", getInputPorts, "get name of input ports of module with ID `arg1`");
    m.def("getOutputPorts", getOutputPorts, "get name of input ports of module with ID `arg1`");
    m.def("getPortDescription", getPortDescription,
          "get description of port with name `arg2` of module with ID `arg1`");
    m.def("waitForHub", waitForNamedHub, "wait for slave hub named `arg1` to connect");
    m.def("waitForHub", waitForAnySlaveHub, "wait for any additional slave hub to connect");
    m.def("waitForHubs", waitForSlaveHubs, "wait for `count` additional slave hubs to connect");
    m.def("waitForNamedHubs", waitForNamedHubs, "wait for named hubs to connect");
    m.def("getMasterHub", getMasterHub, "get ID of master hub");
    m.def("getVistleSession", getVistleSession, "get ID for Vistle session");
    m.def("getAllHubs", getAllHubs, "get ID of all known hubs");
    m.def("getHub", getHub, "get ID of hub for module with ID `arg1`");
    m.def("getConnections", getConnections, "get connections to/from port `arg2` of module with ID `arg1`");
    m.def("getParameters", getParameters, "get list of parameters for module with ID `arg1`");
    m.def("getParameterType", getParameterType, "get type of parameter named `arg2` of module with ID `arg1`");
    m.def("getParameterPresentation", getParameterPresentation,
          "get presentation of parameter named `arg2` of module with ID `arg1`");
    m.def("isParameterDefault", isParameterDefault,
          "check whether parameter `arg2` of module with ID `arg1` differs from its default value");
    m.def("getIntParam", getParameterValue<Integer>, "get value of parameter named `arg2` of module with ID `arg1`");
    m.def("getFloatParam", getParameterValue<Float>, "get value of parameter named `arg2` of module with ID `arg1`");
    m.def("getVectorParam", getParameterValue<ParamVector>,
          "get value of parameter named `arg2` of module with ID `arg1`");
    m.def("getIntVectorParam", getParameterValue<IntParamVector>,
          "get value of parameter named `arg2` of module with ID `arg1`");
    m.def("getStringParam", getParameterValue<std::string>,
          "get value of parameter named `arg2` of module with ID `arg1`");
    m.def("getEscapedStringParam", getEscapedStringParam,
          "get value of parameter named `arg2` of module with ID `arg1`");
    m.def("getParameterTooltip", getParameterTooltip, "get a short description of the parameter");

#ifndef EMBED_PYTHON
    m.def("sessionConnect", &sessionConnect, "connect to running Vistle instance", "host"_a = "localhost",
          "port"_a = 31093);
    m.def("sessionConnect", &sessionConnectWithObserver, "connect to running Vistle instance", "observer"_a, "host"_a,
          "port"_a);
    m.def("sessionDisconnect", &sessionDisconnect, "disconnect from Vistle");
#endif

    py::bind_vector<ParameterVector<Float>>(m, "ParameterVector<Float>");
    py::bind_vector<ParameterVector<Integer>>(m, "ParameterVector<Integer>");
}

PythonModule::PythonModule(VistleConnection *vc): m_vistleConnection(vc)
{
    assert(pythonModuleInstance == nullptr);
    pythonModuleInstance = this;
    std::cerr << "creating Vistle python module" << std::endl;

    //auto mod = py::module::import("_vistle");
}

PythonModule::~PythonModule()
{
    pythonModuleInstance = nullptr;
}

VistleConnection &PythonModule::vistleConnection() const
{
    assert(m_vistleConnection);
    return *m_vistleConnection;
}

bool PythonModule::import(py::object *ns, const std::string &path)
{
#ifdef EMBED_PYTHON
    // load vistle.py
    try {
        py::dict locals;
        locals["modulename"] = "vistle";
        locals["path"] = path + "/vistle.py";
        std::cerr << "Python: loading " << path + "/vistle.py" << std::endl;
#if PY_MAJOR_VERSION > 3 || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 6)
        py::eval<py::eval_statements>(R"(
         import importlib
         import importlib.util
         spec = importlib.util.spec_from_file_location(modulename, path)
         if spec != None and spec.loader != None:
             newmodule = spec.loader.load_module()
         )",
                                      *ns, locals);
#else
        py::eval<py::eval_statements>(R"(
         import imp
         newmodule = imp.load_module(modulename, open(path), path, ('py', 'U', imp.PY_SOURCE))
         )",
                                      *ns, locals);
#endif
        (*ns)["vistle"] = locals["newmodule"];
    } catch (py::error_already_set &ex) {
        std::cerr << "loading of vistle.py failed: " << ex.what() << std::endl;
        if (PyErr_Occurred()) {
            std::cerr << PythonInterface::errorString() << std::endl;
        }
        //py::handle_exception();
        PyErr_Clear();
        return false;
    }
    if (!PythonInterface::the().exec("from vistle import *")) {
        std::cerr << "importing vistle.py Python add-on failed" << std::endl;
        PyErr_Print();
        return false;
    }

    std::cerr << "done loading of vistle.py" << std::endl;
#endif

    return true;
}

} // namespace vistle
