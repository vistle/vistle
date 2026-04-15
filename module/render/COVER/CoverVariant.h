#ifndef VISTLE_COVER_COVERVARIANT_H
#define VISTLE_COVER_COVERVARIANT_H

#include <string>

#include <osg/Group>
#include <osg/Sequence>

#include <VistlePluginUtil/VistleRenderObject.h>

struct Variant {
    std::string variant;
    std::string name;
    osg::ref_ptr<osg::Group> root;
    osg::ref_ptr<osg::Group> constant;
    osg::ref_ptr<osg::Sequence> animated;
    VariantRenderObject ro;

    Variant(const std::string &basename, const std::string &variant = std::string());
};

#endif
