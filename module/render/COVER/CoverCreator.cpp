#include "CoverCreator.h"

#include "COVER.h"

#include <cover/coVRPluginList.h>

using namespace vistle;

Creator::Creator(int id, const std::string &name, osg::ref_ptr<osg::Group> parent)
: id(id), name(name), baseVariant(name)
{
    parent->addChild(baseVariant.root);
    opencover::coVRPluginList::instance()->addNode(baseVariant.root, nullptr, COVER::the()->plugin());
}

bool Creator::empty() const
{
    if (!variants.empty())
        return false;

    if (baseVariant.animated->getNumChildren() > 0)
        return false;
    if (baseVariant.constant->getNumChildren() > 0)
        return false;

    return true;
}

const Variant &Creator::getVariant(const std::string &variantName,
                                   vistle::RenderObject::InitialVariantVisibility vis) const
{
    if (variantName.empty() || variantName == "NULL")
        return baseVariant;

    auto it = variants.find(variantName);
    if (it == variants.end()) {
        it = variants.emplace(std::make_pair(variantName, Variant(name, variantName))).first;
        if (vis != vistle::RenderObject::DontChange)
            it->second.ro.setInitialVisibility(vis);
        baseVariant.constant->addChild(it->second.root);
        opencover::coVRPluginList::instance()->addNode(it->second.root, &it->second.ro, COVER::the()->plugin());
    }
    return it->second;
}

bool Creator::removeVariant(const std::string &variantName)
{
    osg::ref_ptr<osg::Group> root;

    if (variantName.empty() || variantName == "NULL") {
        root = baseVariant.root;
    } else {
        auto it = variants.find(variantName);
        if (it != variants.end()) {
            root = it->second.root;
            variants.erase(it);
        }
    }

    if (!root)
        return false;

    opencover::coVRPluginList::instance()->removeNode(root, true, root);
    while (root->getNumParents() > 0) {
        root->getParent(0)->removeChild(root);
    }

    return true;
}

osg::ref_ptr<osg::Group> Creator::root(const std::string &variant) const
{
    return getVariant(variant).root;
}

osg::ref_ptr<osg::Group> Creator::constant(const std::string &variant) const
{
    return getVariant(variant).constant;
}

osg::ref_ptr<osg::Sequence> Creator::animated(const std::string &variant) const
{
    return getVariant(variant).animated;
}
