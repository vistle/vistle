#include "CoverVariant.h"

#include <sstream>

#include <osg/ClipNode>
#include <PluginUtil/StaticSequence.h>

Variant::Variant(const std::string &basename, const std::string &variant): variant(variant), ro(variant)
{
    std::stringstream s;
    s << basename;
    if (!variant.empty())
        s << "/" << variant;
    name = s.str();

    if (variant.empty()) {
        root = new osg::ClipNode;
    } else {
        root = new osg::Group;
    }
    root->setName(name);

    constant = new osg::Group;
    constant->setName(name + "/static");
    root->addChild(constant);

    animated = new StaticSequence;
    animated->setName(name + "/animated");
    root->addChild(animated);
}
