#ifndef VISTLE_COVER_COVERCREATOR_H
#define VISTLE_COVER_COVERCREATOR_H

#include <map>
#include <string>

#include <osg/Group>
#include <osg/Sequence>

#include <vistle/renderer/renderobject.h>

#include "CoverVariant.h"

struct Creator {
    Creator(int id, const std::string &name, osg::ref_ptr<osg::Group> parent);

    const Variant &
    getVariant(const std::string &variantName,
               vistle::RenderObject::InitialVariantVisibility vis = vistle::RenderObject::DontChange) const;
    osg::ref_ptr<osg::Group> root(const std::string &variant = std::string()) const;
    osg::ref_ptr<osg::Group> constant(const std::string &variant = std::string()) const;
    osg::ref_ptr<osg::Sequence> animated(const std::string &variant = std::string()) const;
    bool removeVariant(const std::string &variant = std::string());
    bool empty() const;

    int id;
    std::string name;
    Variant baseVariant;
    mutable std::map<std::string, Variant> variants;
};

#endif
