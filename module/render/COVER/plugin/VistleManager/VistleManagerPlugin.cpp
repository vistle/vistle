#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/exceptions.hpp>

// cover
#include <net/message.h>
#include <cover/coVRPluginSupport.h>
#include <cover/coCommandLine.h>
#include <cover/OpenCOVER.h>
#include <cover/coVRMSController.h>
#include <PluginUtil/PluginMessageTypes.h>

#include <VistlePluginUtil/VistleMessage.h>

#include <COVER.h>

// vistle
#include <vistle/util/exception.h>
#include <vistle/util/threadname.h>
#include <vistle/core/statetracker.h>
#include <vistle/control/vistleurl.h>

#include <vistle/manager/manager.h>
#include <vistle/manager/executor.h>
#include <vistle/manager/communicator.h>
#include <vistle/manager/clustermanager.h>
#include <vistle/manager/run_on_main_thread.h>


#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>

#include <cover/ui/Owner.h>
#include <cover/ui/Menu.h>
#include <cover/ui/Action.h>

#include <atomic>

using namespace opencover;
using namespace vistle;


class Vistle: public Executor {
public:
    Vistle(int argc, char *argv[], boost::mpi::communicator comm): Executor(argc, argv, comm) {}
    bool config(int argc, char *argv[]) override
    {
        const char *VISTLE_ROOT = getenv("VISTLE_ROOT");
        const char *VISTLE_BUILDTYPE = getenv("VISTLE_BUILDTYPE");
        setVistleRoot(VISTLE_ROOT ? VISTLE_ROOT : "", VISTLE_BUILDTYPE ? VISTLE_BUILDTYPE : "");
        return true;
    }
};


class VistleManagerPlugin: public opencover::coVRPlugin, public ui::Owner {
public:
    VistleManagerPlugin();
    ~VistleManagerPlugin() override;
    bool init() override;
    bool init2() override;
    bool destroy() override;
    void notify(NotificationLevel level, const char *text) override;
    bool update() override;
    void requestQuit(bool killSession) override;
    void message(int toWhom, int type, int length, const void *data) override;
    std::string collaborativeSessionId() const override;

    void updateSessionUrl(const std::string &url);

private:
    template<class Payload>
    bool sendVistle(const vistle::message::Message &msg, const Payload &payload);
    bool sendVistle(const vistle::message::Message &msg);
    boost::mpi::communicator m_comm;

    COVER *m_module = nullptr;
    coVRPlugin *m_plugin = nullptr;

    std::thread m_thread;
    std::atomic<bool> m_done = false;
};

template<>
bool VistleManagerPlugin::sendVistle(const vistle::message::Message &msg, const vistle::MessagePayload &payload)
{
    message::Buffer buf(msg);
    std::cerr << "sending " << buf << std::endl;
    //std::unique_lock<Communicator> guard(Communicator::the());
    //return Communicator::the().sendMessage(vistle::message::Id::Broadcast, buf, -1, payload);
    return Communicator::the().sendHub(buf, payload);
}

template<class Payload>
bool VistleManagerPlugin::sendVistle(const vistle::message::Message &msg, const Payload &payload)
{
    message::Buffer buf(msg);
    auto data = addPayload(buf, payload);
    MessagePayload pl(data);
    return sendVistle(buf, pl);
}

bool VistleManagerPlugin::sendVistle(const vistle::message::Message &msg)
{
    return sendVistle(msg, MessagePayload());
}


class CoverVistleObserver: public vistle::StateObserver {
    VistleManagerPlugin *m_plug = nullptr;

public:
    CoverVistleObserver(VistleManagerPlugin *plug): m_plug(plug) {}
    void sessionUrlChanged(const std::string &url) override { m_plug->updateSessionUrl(url); }
};

void VistleManagerPlugin::updateSessionUrl(const std::string &url)
{
    vistle::ConnectionData cd;
    if (VistleUrl::parse(url, cd)) {
        std::cerr << "session URL changed: " << url << std::endl;
        if (!cd.conference_url.empty()) {
            cover->addPlugin("Browser");
            cover->sendMessage(this, coVRPluginSupport::TO_ALL, opencover::PluginMessageTypes::Browser,
                               cd.conference_url.length(), cd.conference_url.c_str());
        }
    }
}

VistleManagerPlugin::VistleManagerPlugin(): coVRPlugin("VistleManager"), ui::Owner("VistleManager", cover->ui)
{}

VistleManagerPlugin::~VistleManagerPlugin()
{}

bool VistleManagerPlugin::init()
{
    set_manager_in_cover_plugin();
#ifdef VISTLE_USE_MPI
    m_comm = boost::mpi::communicator(coVRMSController::instance()->getAppCommunicator(), boost::mpi::comm_duplicate);
#else
    m_comm = boost::mpi::communicator(boost::mpi::communicator(), boost::mpi::comm_duplicate);
#endif
    auto conn = getenv("VISTLE_CONNECTION");
    if (!conn) {
        std::cerr << "did not find VISTLE_CONNECTION environment variable" << std::endl;
        return false;
    }

    if (!cover->visMenu) {
        cover->visMenu = new ui::Menu("Vistle", this);

        auto executeButton = new ui::Action("Execute", cover->visMenu);
        cover->visMenu->add(executeButton, ui::Container::KeepFirst);
        executeButton->setShortcut("e");
        executeButton->setCallback([this]() {
            message::Execute exec; // execute all sources in dataflow graph
            exec.setDestId(message::Id::MasterHub);
            sendVistle(exec);
        });
        executeButton->setIcon("view-refresh");
        executeButton->setPriority(ui::Element::Toolbar);
    }

    std::string hostname;
    unsigned short port = 0, dataport = 0, datamgrport;
    std::stringstream str(conn);
    str >> hostname >> port >> dataport >> datamgrport;
    std::cerr << "connecting to hub " << hostname << ":" << port << ", data port: " << dataport
              << ", data manager port: " << datamgrport << std::endl;

    std::vector<std::string> args;
    args.push_back("COVER");
    args.push_back(hostname);
    args.push_back(std::to_string(port));
    args.push_back(std::to_string(dataport));
    args.push_back(std::to_string(datamgrport));

    m_thread = std::thread([args, this]() {
        setThreadName("vistle:manager");

        try {
            vistle::registerTypes();
            std::vector<char *> argv;
            for (auto &a: args) {
                argv.push_back(const_cast<char *>(a.data()));
            }
            Vistle executor(argv.size(), argv.data(), m_comm);

            auto observer = std::make_unique<CoverVistleObserver>(this);
            auto &state = Communicator::the().clusterManager().state();
            state.registerObserver(observer.get());
            updateSessionUrl(state.sessionUrl());

            std::cerr << "created Vistle executor" << std::endl;
            executor.run();
            state.unregisterObserver(observer.get());
        } catch (vistle::exception &e) {
            std::cerr << "fatal exception: " << e.what() << std::endl << e.where() << std::endl;
            exit(1);
        } catch (std::exception &e) {
            std::cerr << "fatal exception: " << e.what() << std::endl;
            exit(1);
        }

        m_done = true;
    });


    return true;
}

bool VistleManagerPlugin::init2()
{
    return true;
}

bool VistleManagerPlugin::destroy()
{
    Communicator::the().terminate();

    if (m_thread.joinable())
        m_thread.join();

    cover->visMenu = nullptr;

    return true;
}

void VistleManagerPlugin::notify(coVRPlugin::NotificationLevel level, const char *message)
{
    std::vector<char> text(strlen(message) + 1);
    memcpy(text.data(), message, text.size());
    if (text.size() > 1 && text[text.size() - 2] == '\n')
        text[text.size() - 2] = '\0';
    if (text[0] == '\0')
        return;
    std::cerr << text.data() << std::endl;

    message::SendText::TextType type = message::SendText::Info;
    switch (level) {
    case coVRPlugin::Info:
        type = message::SendText::Info;
        break;
    case coVRPlugin::Warning:
        type = message::SendText::Warning;
        break;
    case coVRPlugin::Error:
    case coVRPlugin::Fatal:
        type = message::SendText::Error;
        break;
    }
    message::SendText info(type);
    message::SendText::Payload pl(text.data());
    sendVistle(info, pl);
}

bool VistleManagerPlugin::update()
{
    if (m_done) {
        OpenCOVER::instance()->requestQuit();
    }

    if (COVER::the() != m_module) {
        m_module = COVER::the();
        if (m_module) {
            m_plugin = cover->addPlugin("Vistle");
            std::cerr << "VistleManager: loading Vistle plugin" << std::endl;
        } else {
            if (cover->getPlugin("Vistle")) {
                std::cerr << "VistleManager: Vistle plugin still loaded" << std::endl;
                std::cerr << "VistleManager: removing Vistle plugin" << std::endl;
                cover->removePlugin("Vistle");
            }
            m_plugin = nullptr;
        }
        return true;
    }
    return false;
}

void VistleManagerPlugin::requestQuit(bool killSession)
{
    if (killSession) {
        message::Quit quit;
        sendVistle(quit);
    }
    m_comm.barrier();
}

void VistleManagerPlugin::message(int toWhom, int type, int length, const void *data)
{
    if (type == opencover::PluginMessageTypes::VistleMessageOut) {
        const auto *wrap = static_cast<const VistleMessage *>(data);
        sendVistle(wrap->buf, wrap->payload);
    }
}

std::string VistleManagerPlugin::collaborativeSessionId() const
{
    if (m_plugin)
        return m_plugin->collaborativeSessionId();
    return std::string();
}

COVERPLUGIN(VistleManagerPlugin)
