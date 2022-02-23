#ifndef RENDERER_H
#define RENDERER_H

#include <vistle/module/module.h>
#include <vistle/util/enum.h>
#include "renderobject.h"
#include "export.h"

namespace vistle {

DEFINE_ENUM_WITH_STRING_CONVERSIONS(RenderMode, (LocalOnly)(MasterOnly)(AllRanks)(LocalShmLeader)(AllShmLeaders))

class V_RENDEREREXPORT Renderer: public Module {
public:
    Renderer(const std::string &name, const int moduleID, mpi::communicator comm);
    virtual ~Renderer();

    bool dispatch(bool block = true, bool *messageReceived = nullptr, unsigned int minPrio = 0) override;

    int numTimesteps() const;
    void getBounds(Vector3 &min, Vector3 &max);
    void getBounds(Vector3 &min, Vector3 &max, int time);

    struct Variant {
        friend class boost::serialization::access;

        Variant(const std::string &name = std::string()): name(name) {}

        std::string name;
        int objectCount = 0;
        RenderObject::InitialVariantVisibility visible = RenderObject::DontChange;
    };
    typedef std::map<std::string, Variant> VariantMap;
    const VariantMap &variants() const;

    struct ColorMap {
        int sender = vistle::message::Id::Invalid;
        std::string senderPort;
        vistle::Texture1D::const_ptr texture;
    };

    typedef std::map<std::string, ColorMap> ColorMapMap;

protected:
    bool handleMessage(const message::Message *message, const MessagePayload &payload) override;

    virtual bool addColorMap(const std::string &species, Object::const_ptr cmap);
    virtual bool removeColorMap(const std::string &species);

    virtual std::shared_ptr<RenderObject> addObject(int senderId, const std::string &senderPort,
                                                    Object::const_ptr container, Object::const_ptr geom,
                                                    Object::const_ptr normal, Object::const_ptr texture) = 0;
    virtual void removeObject(std::shared_ptr<RenderObject> ro);

    bool changeParameter(const Parameter *p) override;
    void connectionRemoved(const Port *from, const Port *to) override;

    int m_fastestObjectReceivePolicy;
    void removeAllObjects();

    bool m_maySleep = true;
    bool m_replayFinished = false;

private:
    virtual bool render();

    bool handleAddObject(const message::AddObject &add);

    bool addInputObject(int sender, const std::string &senderPort, const std::string &portName,
                        vistle::Object::const_ptr object) override;
    std::shared_ptr<RenderObject> addObjectWrapper(int senderId, const std::string &senderPort,
                                                   Object::const_ptr container, Object::const_ptr geom,
                                                   Object::const_ptr normal, Object::const_ptr texture);
    void removeObjectWrapper(std::shared_ptr<RenderObject> ro);

    void removeAllSentBy(int sender, const std::string &senderPort);

    struct Creator {
        Creator(int id, const std::string &port, const std::string &basename)
        : module(id), port(port), age(0), iteration(-1)
        {
            std::stringstream s;
            s << basename << "_" << module;
            name = s.str();
        }
        bool operator<(const Creator &other) const
        {
            if (module == other.module)
                return port < other.port;
            return module < other.module;
        }
        bool operator==(const Creator &other) const
        {
            if (module == other.module)
                return port == other.port;
            return false;
        }

        int module;
        std::string port;
        mutable int age = 0;
        mutable int iteration = -1;
        std::string name;
    };
    typedef std::set<Creator> CreatorMap;
    CreatorMap m_creatorMap;

    std::vector<std::vector<std::shared_ptr<RenderObject>>> m_objectList;
    IntParameter *m_renderMode = nullptr;
    IntParameter *m_objectsPerFrame = nullptr;
    bool needsSync(const message::Message &m) const;

    VariantMap m_variants;
    ColorMapMap m_colormaps;

    int m_numObjectsPerFrame = 500;
};

} // namespace vistle

#endif
