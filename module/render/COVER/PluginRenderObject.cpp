#include "PluginRenderObject.h"

PluginRenderObject::PluginRenderObject(int senderId, const std::string &senderPort, vistle::Object::const_ptr container,
                                       vistle::Object::const_ptr geometry, vistle::Object::const_ptr normals,
                                       vistle::Object::const_ptr texture)
: vistle::RenderObject(senderId, senderPort, container, geometry, normals, texture)
{}

PluginRenderObject::~PluginRenderObject() = default;
