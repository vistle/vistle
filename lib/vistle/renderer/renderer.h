#ifndef VISTLE_RENDERER_RENDERER_H
#define VISTLE_RENDERER_RENDERER_H

#include <vistle/module/module.h>
#include <vistle/util/enum.h>
#include <vistle/core/message/colormap.h>
#include "renderobject.h"
#include "export.h"

namespace vistle {

DEFINE_ENUM_WITH_STRING_CONVERSIONS(RenderMode, (LocalOnly)(MasterOnly)(AllRanks)(LocalShmLeader)(AllShmLeaders))

class RhrController;

class V_RENDEREREXPORT Renderer: public Module {
    friend class RhrController;

public:
    Renderer(const std::string &name, const int moduleID, mpi::communicator comm);

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
        std::vector<vistle::RGBA> rgba;
    };

    typedef std::map<ColorMapKey, ColorMap> ColorMapMap;

    //! let module clear cache at appropriate times and manage its deletion
    template<class CacheData>
    vistle::ResultCache<CacheData> *getOrCreateGeometryCache(int senderId, const std::string &senderPort)
    {
        typedef ResultCache<CacheData> RC;
        SendPort c(senderId, senderPort);
        auto it = m_geometryCaches.find(c);
        if (it == m_geometryCaches.end()) {
            it = m_geometryCaches.emplace(c, new RC).first;
        }
        assert(dynamic_cast<RC *>(it->second.get()));
        return static_cast<RC *>(it->second.get());
    }

protected:
    bool handleMessage(const message::Message *message, const MessagePayload &payload) override;

    virtual bool addColorMap(const vistle::message::Colormap &cm, std::vector<vistle::RGBA> &rgba);
    virtual bool removeColorMap(const std::string &species, int sourceModule = vistle::message::Id::Invalid);

    virtual std::shared_ptr<RenderObject> addObject(int senderId, const std::string &senderPort,
                                                    Object::const_ptr container, Object::const_ptr geom,
                                                    Object::const_ptr normal, Object::const_ptr mapped) = 0;
    virtual void removeObject(std::shared_ptr<RenderObject> ro);

    bool changeParameter(const Parameter *p) override;
    void connectionRemoved(const Port *from, const Port *to) override;

    int m_fastestObjectReceivePolicy;
    void removeAllObjects();

    bool m_maySleep = true;
    bool m_replayFinished = false;
    vistle::Port *m_dataIn = nullptr;

private:
    virtual bool render();

    bool handleAddObject(const message::AddObject &add);

    bool addInputObject(int sender, const std::string &senderPort, const std::string &portName,
                        vistle::Object::const_ptr object) override;
    std::shared_ptr<RenderObject> addObjectWrapper(int senderId, const std::string &senderPort,
                                                   Object::const_ptr container, Object::const_ptr geom,
                                                   Object::const_ptr normal, Object::const_ptr mapped);
    void removeObjectWrapper(std::shared_ptr<RenderObject> ro);

    void removeAllSentBy(int sender, const std::string &senderPort);

    struct SendPort {
        SendPort(int id, const std::string &port, const std::string &basename = std::string())
        : module(id), port(port), age(0), iteration(-1)
        {
            std::stringstream s;
            s << basename << "_" << module;
            name = s.str();
        }
        bool operator<(const SendPort &other) const
        {
            if (module == other.module)
                return port < other.port;
            return module < other.module;
        }
        bool operator==(const SendPort &other) const
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
    typedef std::set<SendPort> SenderPortMap;
    SenderPortMap m_sendPortMap;

    std::vector<std::vector<std::shared_ptr<RenderObject>>> m_objectList;
    IntParameter *m_renderMode = nullptr;
    IntParameter *m_objectsPerFrame = nullptr;
    bool needsSync(const message::Message &m) const;

    VariantMap m_variants;
    ColorMapMap m_colormaps;

    int m_numObjectsPerFrame = 500;

    void enableGeometryCaches(bool on);
    std::map<SendPort, std::unique_ptr<ResultCacheBase>> m_geometryCaches;
    IntParameter *m_useGeometryCaches = nullptr;
};

} // namespace vistle

#endif
