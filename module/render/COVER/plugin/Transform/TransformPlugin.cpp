#include "TransformInteraction.h"
#include "TransformPlugin.h"
#include <cover/coInteractor.h>
#include <cover/coVRPluginSupport.h>
#include <cover/RenderObject.h>

using namespace covise;
using namespace grmsg;
using namespace opencover;

TransformPlugin::TransformPlugin(): ModuleFeedbackPlugin("Transform")
{}

void TransformPlugin::preFrame()
{
    ModuleFeedbackPlugin::preFrame();
}

void TransformPlugin::addNode(osg::Node *node, const RenderObject *obj)
{
    if (obj) {
        addNodeToCase(obj->getName(), node);
    }
}

ModuleFeedbackManager *TransformPlugin::NewModuleFeedbackManager(const RenderObject *container,
                                                                 coInteractor *interactor, const RenderObject *,
                                                                 const char *pluginName)
{
    return new TransformInteraction(container, interactor, pluginName, this);
}

// called whenever cover receives a Vistle object
// with feedback info appended
void TransformPlugin::newInteractor(const RenderObject *container, coInteractor *i)
{
    const char *moduleName = i->getModuleName();
    if (strncmp(moduleName, "Transform", 9) == 0) {
        add(container, i);
    }

    ModuleFeedbackPlugin::newInteractor(container, i);
}

void TransformPlugin::removeObject(const char *objName, bool r)
{
    if (cover->debugLevel(2))
        fprintf(stderr, "\n--- TransformPlugin::removeObject objectName=[%s]\n", objName);

    // replace is handled in addObject
    if (!r) {
        remove(objName);
    }
}

COVERPLUGIN(TransformPlugin)
