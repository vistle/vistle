#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/exceptions.hpp>

// cover
#include <net/message.h>
#include <cover/coVRPluginSupport.h>
#include <cover/coCommandLine.h>
#include <cover/OpenCOVER.h>
#include <PluginUtil/PluginMessageTypes.h>

#include <VistlePluginUtil/VistleMessage.h>
#include <VistlePluginUtil/VistleInfo.h>
#include <VistlePluginUtil/VistleInteractor.h>

#include <COVER.h>

// vistle
#include <vistle/util/exception.h>
#include <vistle/util/meta.h>
#include <vistle/core/statetracker.h>
#include <vistle/control/vistleurl.h>

#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>

#include <cover/ui/Owner.h>
#include <cover/ui/Menu.h>
#include <cover/ui/Action.h>
#include <cover/ui/Group.h>
#include <cover/ui/Button.h>
#include <cover/ui/SelectionList.h>
#include <cover/ui/Slider.h>
#include <PluginUtil/coVR3DTransInteractor.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <string>

using namespace opencover;
using namespace vistle;

class CoverVistleObserver;
class VistlePlugin: public opencover::coVRPlugin, public ui::Owner {
    friend CoverVistleObserver;

public:
    VistlePlugin();
    ~VistlePlugin() override;
    bool init() override;
    bool init2() override;
    bool destroy() override;
    void notify(NotificationLevel level, const char *text) override;
    bool update() override;
    void requestQuit(bool killSession) override;
    void message(int toWhom, int type, int length, const void *data) override;
    bool sendVisMessage(const covise::Message *msg) override;
    std::string collaborativeSessionId() const override;
    void newInteractor(const opencover::RenderObject *container, coInteractor *it) override;
    void removeObject(const char *objName, bool replaceFlag) override;

    void updateSessionUrl(const std::string &url);
    void showGuiForParameter(int moduleId, const std::string &parameterName, bool show);

private:
    enum class ControlType {
        Invalid,
        Boolean,
        Choice,
        IntSlider,
        FloatSlider,
        FloatVector,
    };

    struct ParameterControl {
        ControlType type = ControlType::Invalid;
        ui::Button *button = nullptr;
        ui::SelectionList *choice = nullptr;
        ui::Slider *slider = nullptr;
        std::unique_ptr<coVR3DTransInteractor> vectorInteractor;
        bool showVectorInteractor = false;
    };

    struct ModuleControls {
        ui::Menu *submenu = nullptr;
        std::map<std::string, ParameterControl> controls;
    };

    VistleInteractor *interactorForModule(int moduleId) const;
    bool isSupportedVrParameter(const std::shared_ptr<vistle::Parameter> &param) const;
    void removeVrControl(int moduleId, const std::string &name);
    void showVrControl(const std::string &moduleName, const std::shared_ptr<vistle::Parameter> &param);
    void updateVrControl(const std::shared_ptr<vistle::Parameter> &param, ParameterControl &control);
    void preFrameVectorInteractors();
    void cleanupRemovedModules();
    void updateMenuNames();

    std::map<int, ModuleControls> m_moduleControls;

    COVER *m_module = nullptr;
    std::unique_ptr<vistle::StateObserver> m_observer;
};


class CoverVistleObserver: public vistle::StateObserver {
    VistlePlugin *m_plug = nullptr;

public:
    CoverVistleObserver(VistlePlugin *plug): m_plug(plug) {}
    void sessionUrlChanged(const std::string &url) override { m_plug->updateSessionUrl(url); }
    void parameterConfigChanged(int moduleId, const std::string &parameterName,
                                vistle::Parameter::ConfigurationType configType) override
    {
        if (configType == vistle::Parameter::RendererGui) {
            if (auto param = m_plug->m_module->state().getParameter(moduleId, parameterName)) {
                m_plug->showGuiForParameter(moduleId, parameterName, param->configuration(configType, 0));
            }
        }
    }
    void parameterChoicesChanged(int moduleId, const std::string &parameterName) override
    {
        auto param = m_plug->m_module->state().getParameter(moduleId, parameterName);
        if (param && param->configuration(vistle::Parameter::RendererGui, 0)) {
            m_plug->updateVrControl(param, m_plug->m_moduleControls[moduleId].controls[parameterName]);
        }
    }
};

void VistlePlugin::updateSessionUrl(const std::string &url)
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

void VistlePlugin::showGuiForParameter(int moduleId, const std::string &parameterName, bool show)
{
    if (!m_module || !cover->visMenu)
        return;

    auto inter = interactorForModule(moduleId);
    auto param = m_module->state().getParameter(moduleId, parameterName);
    if (!param)
        return;
    if (show)
        showVrControl(inter->getModuleDisplayName(), param);
    else
        removeVrControl(moduleId, parameterName);
}

VistleInteractor *VistlePlugin::interactorForModule(int moduleId) const
{
    if (!m_module)
        return nullptr;

    auto it = m_module->m_interactorMap.find(moduleId);
    if (it == m_module->m_interactorMap.end())
        return nullptr;

    return it->second;
}

bool VistlePlugin::isSupportedVrParameter(const std::shared_ptr<vistle::Parameter> &param) const
{
    if (!param)
        return false;
    if (!param->configuration(Parameter::RendererGui, 0))
        return false;
    if (param->type() == Parameter::String || param->type() == Parameter::StringVector)
        return false;
    return true;
}

void VistlePlugin::removeVrControl(int moduleId, const std::string &name)
{
    auto mit = m_moduleControls.find(moduleId);
    if (mit == m_moduleControls.end())
        return;

    auto &moduleControls = mit->second;
    auto cit = moduleControls.controls.find(name);
    if (cit != moduleControls.controls.end()) {
        auto &control = cit->second;
        if (control.vectorInteractor) {
            control.vectorInteractor->hide();
            control.vectorInteractor->disableIntersection();
            control.vectorInteractor.reset();
        }
        moduleControls.controls.erase(cit);
    }

    if (moduleControls.controls.empty() && moduleControls.submenu && cover->visMenu) {
        cover->visMenu->remove(moduleControls.submenu);
        delete moduleControls.submenu;
        moduleControls.submenu = nullptr;
    }

    if (moduleControls.controls.empty() && !moduleControls.submenu) {
        m_moduleControls.erase(mit);
    }
}

void VistlePlugin::updateVrControl(const std::shared_ptr<vistle::Parameter> &param, ParameterControl &control)
{
    if (!param)
        return;

    switch (control.type) {
    case ControlType::Boolean: {
        auto iparam = std::dynamic_pointer_cast<IntParameter>(param);
        if (iparam && control.button) {
            control.button->setState(iparam->getValue() != 0);
        }
        break;
    }
    case ControlType::Choice: {
        auto iparam = std::dynamic_pointer_cast<IntParameter>(param);
        if (iparam && control.choice) {
            control.choice->setList(iparam->choices());
            const int n = static_cast<int>(iparam->choices().size());
            const int active = std::clamp((int)iparam->getValue(), 0, std::max(0, n - 1));
            control.choice->select(active);
        }
        break;
    }
    case ControlType::IntSlider: {
        auto iparam = std::dynamic_pointer_cast<IntParameter>(param);
        if (iparam && control.slider) {
            control.slider->setValue(iparam->getValue());
        }
        break;
    }
    case ControlType::FloatSlider: {
        auto fparam = std::dynamic_pointer_cast<FloatParameter>(param);
        if (fparam && control.slider) {
            control.slider->setValue(fparam->getValue());
        }
        break;
    }
    case ControlType::FloatVector: {
        auto vparam = std::dynamic_pointer_cast<VectorParameter>(param);
        if (vparam && control.vectorInteractor) {
            const auto value = vparam->getValue();
            if (value.dim >= 3) {
                control.vectorInteractor->updateTransform(osg::Vec3(value[0], value[1], value[2]));
            }
        }
        if (control.button) {
            control.button->setState(control.showVectorInteractor);
        }
        break;
    }
    case ControlType::Invalid:
        break;
    }
}

template<typename T>
ui::Slider *createSlider(const vistle::ParameterBase<T> &param, opencover::ui::Group *group,
                         const std::string &elementName)
{
    auto value = param.getValue();
    T min = 0;
    T max = 100;
    if (param.minimum() <= std::numeric_limits<T>::min() || param.maximum() >= std::numeric_limits<T>::max()) {
        min = value - 50;
        max = value + 50;
    } else {
        min = param.minimum();
        max = param.maximum();
    }

    auto slider = new ui::Slider(group, elementName);
    if (std::is_integral<T>::value)
        slider->setIntegral(true);
    slider->setBounds(min, max);

    return slider;
}

void VistlePlugin::showVrControl(const std::string &moduleType, const std::shared_ptr<vistle::Parameter> &param)
{
    if (!param || !cover->visMenu)
        return;
    auto name = param->getName();
    auto moduleId = param->module();

    if (!isSupportedVrParameter(param)) {
        removeVrControl(moduleId, name);
        return;
    }

    auto &moduleControls = m_moduleControls[moduleId];
    if (!moduleControls.submenu) {
        const std::string submenuName = moduleType + std::to_string(moduleId);
        moduleControls.submenu = new ui::Menu(cover->visMenu, submenuName);
    }
    moduleControls.submenu->setText(moduleType);

    auto &control = moduleControls.controls[name];
    if (control.type != ControlType::Invalid) {
        updateVrControl(param, control);
        return;
    }

    const std::string elementName = "param_" + std::to_string(moduleId) + "_" + name;
    const std::string label = name;
    if (param->presentation() == Parameter::Boolean && param->type() == Parameter::Integer) {
        auto button = new ui::Button(moduleControls.submenu, elementName);
        button->setText(label);
        button->setCallback([this, moduleId, name](bool state) {
            if (auto *i = interactorForModule(moduleId)) {
                i->setBooleanParam(name.c_str(), state ? 1 : 0);
                i->executeModule();
            }
        });
        control.type = ControlType::Boolean;
        control.button = button;
    } else if (param->presentation() == Parameter::Choice && param->type() == Parameter::Integer) {
        auto choice = new ui::SelectionList(moduleControls.submenu, elementName);
        choice->setText(label);
        choice->setList(param->choices());
        choice->setCallback([this, moduleId, name](int pos) {
            if (auto *i = interactorForModule(moduleId)) {
                i->setChoiceParam(name.c_str(), pos);
                i->executeModule();
            }
        });
        control.type = ControlType::Choice;
        control.choice = choice;
    } else if (param->presentation() == Parameter::Slider) {
        opencover::ui::Slider *slider = nullptr;
        using SliderTypes = std::tuple<vistle::IntParameter, vistle::FloatParameter>;
        meta::dispatch_by_type<SliderTypes>(*param, [&]<typename T>() {
            const T *p = std::static_pointer_cast<const T>(param).get();
            slider = createSlider(*p, moduleControls.submenu, elementName);
        });


        slider->setText(label);
        const bool integerSlider = param->type() == Parameter::Integer;
        slider->setCallback([this, moduleId, name, integerSlider](double v, bool released) {
            if (auto *i = interactorForModule(moduleId)) {
                if (integerSlider) {
                    const int value = static_cast<int>(std::lround(v));
                    i->setSliderParam(name.c_str(), 0, 0, value);
                } else {
                    i->setSliderParam(name.c_str(), 0.f, 0.f, static_cast<float>(v));
                }
                if (released)
                    i->executeModule();
            }
        });

        control.type = param->type() == Parameter::Integer ? ControlType::IntSlider : ControlType::FloatSlider;
        control.slider = slider;
    } else if (param->type() == Parameter::Vector) {
        auto vparam = std::dynamic_pointer_cast<VectorParameter>(param);
        assert(vparam);
        if (!vparam) {
            moduleControls.controls.erase(name);
            return;
        }

        const auto value = vparam->getValue();
        if (value.dim != 3) {
            moduleControls.controls.erase(name);
            return;
        }

        auto button = new ui::Button(moduleControls.submenu, elementName + "_show_interactor");
        button->setText(label + ": show interactor");

        osg::Vec3 pos(value[0], value[1], value[2]);
        float interSize = -1.0f;
        auto vectorInteractor =
            std::make_unique<coVR3DTransInteractor>(pos, interSize, vrui::coInteraction::ButtonA, "hand",
                                                    "VistleVectorInteractor", vrui::coInteraction::Medium);
        vectorInteractor->hide();
        vectorInteractor->disableIntersection();

        control.type = ControlType::FloatVector;
        control.button = button;
        control.vectorInteractor = std::move(vectorInteractor);

        button->setCallback([this, moduleId, name](bool state) {
            auto mit = m_moduleControls.find(moduleId);
            if (mit == m_moduleControls.end())
                return;
            auto cit = mit->second.controls.find(name);
            if (cit == mit->second.controls.end())
                return;

            auto &ctrl = cit->second;
            ctrl.showVectorInteractor = state;
            if (!ctrl.vectorInteractor)
                return;

            if (state) {
                ctrl.vectorInteractor->show();
                ctrl.vectorInteractor->enableIntersection();
            } else {
                ctrl.vectorInteractor->hide();
                ctrl.vectorInteractor->disableIntersection();
            }
        });
    } else {
        moduleControls.controls.erase(name);
        return;
    }

    updateVrControl(param, control);
}

void VistlePlugin::cleanupRemovedModules()
{
    if (!m_module)
        return;

    std::set<int> activeModules;
    for (const auto &[moduleId, inter]: m_module->m_interactorMap) {
        if (inter)
            activeModules.insert(moduleId);
    }

    for (auto it = m_moduleControls.begin(); it != m_moduleControls.end();) {
        if (activeModules.find(it->first) != activeModules.end()) {
            ++it;
            continue;
        }

        auto &moduleControls = it->second;
        if (moduleControls.submenu && cover->visMenu) {
            cover->visMenu->remove(moduleControls.submenu);
            delete moduleControls.submenu;
            moduleControls.submenu = nullptr;
        }

        it = m_moduleControls.erase(it);
    }
}

void VistlePlugin::updateMenuNames()
{
    if (!m_module)
        return;

    for (auto it = m_moduleControls.begin(); it != m_moduleControls.end(); ++it) {
        auto &moduleControls = it->second;
        if (moduleControls.submenu) {
            if (auto inter = interactorForModule(it->first))
                moduleControls.submenu->setText(inter->getModuleDisplayName());
        }
    }
}

void VistlePlugin::preFrameVectorInteractors()
{
    for (auto &[moduleId, moduleControls]: m_moduleControls) {
        auto *inter = interactorForModule(moduleId);
        if (!inter)
            continue;

        for (auto &[name, control]: moduleControls.controls) {
            if (control.type != ControlType::FloatVector || !control.vectorInteractor || !control.showVectorInteractor)
                continue;

            control.vectorInteractor->preFrame();
            if (!control.vectorInteractor->wasStopped())
                continue;

            const auto pos = control.vectorInteractor->getPos();
            float value[3] = {pos[0], pos[1], pos[2]};
            inter->setVectorParam(name.c_str(), 3, value);
            inter->executeModule();
        }
    }
}

VistlePlugin::VistlePlugin(): coVRPlugin("Vistle"), ui::Owner("Vistle", cover->ui), m_module(nullptr)
{}

VistlePlugin::~VistlePlugin()
{
    VistleInfo::setModule(nullptr);
    if (m_module) {
        m_module->comm().barrier();
        m_module->prepareQuit();
        m_module = nullptr;
    }
}

bool VistlePlugin::init()
{
    m_module = COVER::the();
    VistleInfo::setModule(m_module);
    m_observer = std::make_unique<CoverVistleObserver>(this);

    if (!m_module) {
        std::cerr << "COVER module not found, aborting Vistle plugin initialization" << std::endl;
        return false;
    }

    m_module->state().registerObserver(m_observer.get());
    updateSessionUrl(m_module->state().sessionUrl());

    if (!cover->visMenu) {
        cover->visMenu = new ui::Menu("Vistle", this);

        auto executeButton = new ui::Action("Execute", cover->visMenu);
        cover->visMenu->add(executeButton, ui::Container::KeepFirst);
        executeButton->setShortcut("e");
        executeButton->setCallback([this]() {
            if (m_module) {
                m_module->executeAll();
            }
        });
        executeButton->setIcon("view-refresh");
        executeButton->setPriority(ui::Element::Toolbar);
    }

    do {
        update();
        if (OpenCOVER::instance()->getExitFlag()) {
            std::cerr << "COVER is exiting, aborting Vistle plugin initialization" << std::endl;
            return false;
        }
    } while (m_module->state().getModuleState(m_module->id()) == vistle::StateObserver::Unknown);

    return true;
}

bool VistlePlugin::init2()
{
    if (m_module) {
        m_module->setPlugin(this);
        return true;
    }
    return false;
}

bool VistlePlugin::destroy()
{
    if (m_module) {
        m_module->state().unregisterObserver(m_observer.get());
        m_module->comm().barrier();
        m_module->prepareQuit();
        m_module->setPlugin(nullptr);
        m_module = nullptr;
    }

    m_observer.reset();

    return true;
}

void VistlePlugin::notify(coVRPlugin::NotificationLevel level, const char *message)
{
    std::vector<char> text(strlen(message) + 1);
    memcpy(text.data(), message, text.size());
    if (text.size() > 1 && text[text.size() - 2] == '\n')
        text[text.size() - 2] = '\0';
    if (text[0] == '\0')
        return;
    std::cerr << text.data() << std::endl;
    if (m_module) {
        switch (level) {
        case coVRPlugin::Info:
            m_module->sendInfo("%s", text.data());
            break;
        case coVRPlugin::Warning:
            m_module->sendWarning("%s", text.data());
            break;
        case coVRPlugin::Error:
        case coVRPlugin::Fatal:
            m_module->sendError("%s", text.data());
            break;
        }
    }
}

bool VistlePlugin::update()
{
#ifndef NDEBUG
    if (m_module) {
        m_module->comm().barrier();
    }
#endif
    bool messageReceived = false;
    try {
        if (m_module && !m_module->dispatch(false, &messageReceived)) {
            std::cerr << "Vistle requested COVER to quit" << std::endl;
            if (cover->getPlugin("VistleManager")) {
                auto mod = m_module;
                m_module = nullptr;
                mod->setPlugin(nullptr);
            } else {
                OpenCOVER::instance()->requestQuit();
            }
        }
    } catch (boost::interprocess::interprocess_exception &e) {
        std::cerr << "VistlePlugin::update: interprocess_exception: " << e.what() << std::endl;
        std::cerr << "   error: code: " << e.get_error_code() << ", native: " << e.get_native_error() << std::endl;
        throw(e);
    } catch (std::exception &e) {
        std::cerr << "VistlePlugin::update: std::exception: " << e.what() << std::endl;
        throw(e);
    }

    bool updateRequested = false;
    if (m_module) {
        preFrameVectorInteractors();
        updateRequested = m_module->updateRequired();
        m_module->clearUpdate();
    }
    return messageReceived || updateRequested;
}

void VistlePlugin::requestQuit(bool killSession)
{
    if (m_module) {
        if (killSession) {
            message::Quit quit;
            m_module->sendMessage(quit);
        }
        m_module->comm().barrier();
        m_module->prepareQuit();
        m_module = nullptr;
    }
}

void VistlePlugin::message(int toWhom, int type, int length, const void *data)
{
    if (type == opencover::PluginMessageTypes::VistleMessageOut) {
        const auto *wrap = static_cast<const VistleMessage *>(data);
        if (m_module) {
            m_module->sendMessage(wrap->buf, wrap->payload);
        }
    }
}

bool VistlePlugin::sendVisMessage(const covise::Message *msg)
{
    if (!m_module) {
        return false;
    }

#if 0
    auto tostr = [](int t) -> std::string {
        std::stringstream str;
        str << t;
        if (t < 0 || t >= covise::COVISE_MESSAGE_LAST_DUMMY_MESSAGE) {
            str << " (invalid)";
        } else {
            str << " (" << covise::covise_msg_types_array[t] << ")";
        }
        return str.str();
    };

    std::cerr << "VistlePlugin::sendVisMessage: sender=" << msg->sender << ", send_type=" << msg->send_type
              << ", type=" << tostr(msg->type) << std::endl;
#endif

    if (msg->type == covise::Message::UI && msg->data.data() &&
        strncmp(msg->data.data(), "WANT_TABLETUI", msg->data.length()) == 0) {
        // no one is listening for this
        return true;
    }

    message::Cover cover(m_module->mirrorId(), msg->sender, msg->send_type, msg->type);
    cover.setDestId(message::Id::MasterHub);
    MessagePayload pl(msg->data.data(), msg->data.length());
    cover.setPayloadSize(msg->data.length());
    return m_module->sendMessage(cover, pl);
}

std::string VistlePlugin::collaborativeSessionId() const
{
    std::string id = "COVER";
    if (!m_module) {
        return id;
    }

    id += "_" + std::to_string(m_module->mirrorId());
    return id;
}

void VistlePlugin::newInteractor(const opencover::RenderObject *container, coInteractor *it)
{
    updateMenuNames();
}

void VistlePlugin::removeObject(const char *objName, bool replaceFlag)
{
    cleanupRemovedModules();
}

COVERPLUGIN(VistlePlugin)
