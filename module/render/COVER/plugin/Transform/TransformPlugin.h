#ifndef VISTLE_COVER_PLUGIN_TRANSFORM_TRANSFORMPLUGIN_H
#define VISTLE_COVER_PLUGIN_TRANSFORM_TRANSFORMPLUGIN_H

#include <PluginUtil/ModuleFeedbackPlugin.h>

// OpenCOVER interaction for Transform module
namespace opencover {

class TransformPlugin: public ModuleFeedbackPlugin {
public:
    TransformPlugin();
    // update scale of interactors and check for changes
    void preFrame() override;

    void removeObject(const char *objName, bool replace) override;

    // this will be called if a COVISE object with feedback arrives
    void newInteractor(const RenderObject *container, coInteractor *i) override;

    void addNode(osg::Node *node, const RenderObject *obj) override;

protected:
    virtual ModuleFeedbackManager *NewModuleFeedbackManager(const RenderObject *container, coInteractor *interactor,
                                                            const RenderObject *geomobj,
                                                            const char *pluginName) override;
};

} // namespace opencover
#endif
