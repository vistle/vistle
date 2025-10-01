#ifndef VISTLE_COVER_PLUGIN_TRANSFORM_TRANSFORMINTERACTION_H
#define VISTLE_COVER_PLUGIN_TRANSFORM_TRANSFORMINTERACTION_H

#include <memory>
#include <PluginUtil/ModuleInteraction.h>

namespace opencover {
class coInteractor;
class coVR3DTransformInteractor;
class RenderObject;
class TransformPlugin;
namespace ui {
class Action;
}

namespace ui {
class Slider;
}

class TransformInteraction: public ModuleInteraction {
private:
    std::unique_ptr<coVR3DTransformInteractor> m_interactor;
    TransformPlugin *m_plugin;
    opencover::ui::Action *m_resetScaleButton;
    opencover::ui::Slider *m_sizeSlider;

public:
    TransformInteraction(const RenderObject *container, coInteractor *inter, const char *pluginName,
                         TransformPlugin *plugin);
    virtual ~TransformInteraction();

    void preFrame() override;
    void update(const RenderObject *container, coInteractor *inter) override;
    void updatePickInteractors(bool show) override;
    void updateDirectInteractors(bool show) override;

private:
    void updateInteractorTransform();
    void sendTransformToModule();
};

} // namespace opencover
#endif
