#ifndef VISTLE_BLACKHOLE_BLACKHOLE_H
#define VISTLE_BLACKHOLE_BLACKHOLE_H

#include <vistle/core/parameter.h>
#include <vistle/renderer/renderer.h>

class BlackHole: public vistle::Renderer {
public:
    BlackHole(const std::string &name, int moduleId, mpi::communicator comm);

    std::shared_ptr<vistle::RenderObject> addObject(int senderId, const std::string &senderPort,
                                                    vistle::Object::const_ptr container,
                                                    vistle::Object::const_ptr geometry,
                                                    vistle::Object::const_ptr normals,
                                                    vistle::Object::const_ptr texture) override;

    vistle::IntParameter *m_keepReferences = nullptr;
};

#endif
