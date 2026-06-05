#include "BlackHole.h"

#include <vistle/core/messages.h>
#include <vistle/core/object.h>
#include <vistle/renderer/renderobject.h>

#include <cstdlib>
#include <cstring>
#include <memory>

using namespace vistle;

BlackHole::BlackHole(const std::string &name, int moduleId, mpi::communicator comm)
: vistle::Renderer(name, moduleId, comm)
{
    m_keepReferences =
        addIntParameter("keep_references", "retain references to input objects", false, Parameter::Boolean);
}

std::shared_ptr<vistle::RenderObject> BlackHole::addObject(int senderId, const std::string &senderPort,
                                                           vistle::Object::const_ptr container,
                                                           vistle::Object::const_ptr geometry,
                                                           vistle::Object::const_ptr normals,
                                                           vistle::Object::const_ptr texture)
{
    if (m_keepReferences->getValue() == 0) {
        return std::make_shared<RenderObject>(senderId, senderPort, nullptr, nullptr, nullptr, nullptr);
    }
    return std::make_shared<RenderObject>(senderId, senderPort, container, geometry, normals, texture);
}

MODULE_MAIN(BlackHole)
