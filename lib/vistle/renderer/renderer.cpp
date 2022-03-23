#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>
#include <vistle/core/statetracker.h>
#include <vistle/core/object.h>
#include <vistle/core/empty.h>
#include <vistle/core/placeholder.h>
#include <vistle/core/coords.h>
#include <vistle/core/layergrid.h>
#include <vistle/core/archives.h>
#include <vistle/core/archive_loader.h>
#include <vistle/core/grid.h>

#include "renderer.h"

#include <vistle/util/vecstreambuf.h>
#include <vistle/util/sleep.h>
#include <vistle/util/stopwatch.h>

namespace mpi = boost::mpi;

namespace vistle {

Renderer::Renderer(const std::string &name, const int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm), m_fastestObjectReceivePolicy(message::ObjectReceivePolicy::Local)
{
    setSchedulingPolicy(message::SchedulingPolicy::Ignore); // compute does not have to be called at all
    setReducePolicy(message::ReducePolicy::Never); // because of COMBINE port
    createInputPort("data_in", "input data", Port::COMBINE);

    m_renderMode = addIntParameter("render_mode", "Render on which nodes?", LocalOnly, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_renderMode, RenderMode);

    m_objectsPerFrame = addIntParameter("objects_per_frame", "Max. no. of objects to load between calls to render",
                                        m_numObjectsPerFrame);
    setParameterMinimum(m_objectsPerFrame, Integer(1));

    //std::cerr << "Renderer starting: rank=" << rank << std::endl;
    m_useGeometryCaches =
        addIntParameter("_use_geometry_cache", "whether to try to cache geometry for re-use in subseqeuent timesteps",
                        true, Parameter::Boolean);
}

Renderer::~Renderer() = default;

// all of those messages have to arrive in the same order an all ranks, but other messages may be interspersed
bool Renderer::needsSync(const message::Message &m) const
{
    using namespace vistle::message;

    switch (m.type()) {
    case CANCELEXECUTE:
    case QUIT:
    case KILL:
    case ADDPARAMETER:
    case SETPARAMETER:
    case REMOVEPARAMETER:
    case REPLAYFINISHED:
        return true;
    case ADDOBJECT:
        return objectReceivePolicy() != ObjectReceivePolicy::Local;
    default:
        return false;
    }
}

std::array<Object::const_ptr, 3> splitObject(Object::const_ptr container)
{
    std::array<Object::const_ptr, 3> geo_norm_data;
    Object::const_ptr &grid = geo_norm_data[0];
    Object::const_ptr &normals = geo_norm_data[1];
    Object::const_ptr &data = geo_norm_data[2];

    if (auto ph = vistle::PlaceHolder::as(container)) {
        grid = ph->geometry();
        normals = ph->normals();
        data = ph->texture();
        return geo_norm_data;
    }

    if (auto t = vistle::Texture1D::as(container)) {
        // do not treat as Vec<Scalar,1>
        data = t;
        grid = t->grid();
    } else if (auto c = vistle::Coords::as(container)) {
        // do not treat as Vec<Scalar,3>
        grid = c;
    } else if (auto l = vistle::LayerGrid::as(container)) {
        // do not treat as Vec<Scalar,1>
        grid = l;
    } else if (auto d = vistle::DataBase::as(container)) {
        data = d;
        grid = d->grid();
    } else {
        grid = container;
    }
    if (grid) {
        if (auto gi = grid->getInterface<GridInterface>()) {
            normals = gi->normals();
        }
    }

    return geo_norm_data;
}

bool Renderer::handleMessage(const message::Message *message, const MessagePayload &payload)
{
    switch (message->type()) {
    case vistle::message::REPLAYFINISHED: {
        m_replayFinished = true;
        break;
    }
    case vistle::message::ADDOBJECT: {
        auto add = static_cast<const message::AddObject *>(message);
        if (payload)
            m_stateTracker->handle(*add, payload->data(), payload->size());
        else
            m_stateTracker->handle(*add, nullptr);
        return handleAddObject(*add);
        break;
    }
    default: {
        break;
    }
    }

    return Module::handleMessage(message, payload);
}

bool Renderer::addColorMap(const std::string &species, Object::const_ptr cmap)
{
    return true;
}

bool Renderer::removeColorMap(const std::string &species)
{
    return true;
}

bool Renderer::handleAddObject(const message::AddObject &add)
{
    using namespace vistle::message;

    const RenderMode rm = static_cast<RenderMode>(m_renderMode->getValue());
    const auto pol = objectReceivePolicy();

    Object::const_ptr obj, placeholder;
    std::vector<Object::const_ptr> objs;
    if (shmLeader(add.rank()) == shmLeader(rank())) {
        obj = add.takeObject();
        assert(obj);
        if (rank() != add.rank())
            obj->ref();
        if (pol == ObjectReceivePolicy::Distribute) {
            m_commShmGroup.barrier();
        }
    }

    if (size() > 1) {
        if (rm == AllRanks || rm == AllShmLeaders) {
            assert(pol == ObjectReceivePolicy::Distribute);
            broadcastObjectViaShm(obj, add.objectName(), add.rank());
            assert(obj);
        }
        if (rm != AllRanks && pol == ObjectReceivePolicy::Distribute) {
            std::string phName;
            if (add.rank() == rank()) {
                auto geo_norm_tex = splitObject(obj);
                auto &grid = geo_norm_tex[0];
                auto &normals = geo_norm_tex[1];
                auto &tex = geo_norm_tex[2];

                auto ph = std::make_shared<PlaceHolder>(obj);
                ph->setGeometry(grid);
                ph->setNormals(normals);
                ph->setTexture(tex);
                placeholder = ph;
                phName = ph->getName();
            }

            mpi::broadcast(comm(), phName, add.rank());
            if (add.rank() != rank() && shmLeader(add.rank()) == shmLeader(rank())) {
                placeholder = Shm::the().getObjectFromName(phName);
                assert(placeholder);
            }
            broadcastObjectViaShm(placeholder, phName, add.rank());
            assert(placeholder);
            m_commShmGroup.barrier();
        }

        if (rm == MasterOnly && shmLeader(add.rank()) != 0) {
            if (rank() == add.rank()) {
                sendObject(obj, 0);
            } else if (rank() == 0) {
                obj = receiveObject(add.rank());
            }
        }
    }

    if (rm == AllRanks || (rm == AllShmLeaders && rank() == shmLeader()) || (rm == MasterOnly && rank() == 0) ||
        (rm == LocalShmLeader && shmLeader(add.rank()) == shmLeader()) || (rm == LocalOnly && rank() == add.rank())) {
        assert(obj);
        addInputObject(add.senderId(), add.getSenderPort(), add.getDestPort(), obj);
    } else if (pol == ObjectReceivePolicy::Distribute) {
        assert(placeholder);
        addInputObject(add.senderId(), add.getSenderPort(), add.getDestPort(), placeholder);
    }

    return true;
}

bool Renderer::dispatch(bool block, bool *messageReceived, unsigned int minPrio)
{
    (void)block;
    int quit = 0;
    bool wasAnyMessage = false;
    int numSync = 0;
    int maxNumMessages = 0;
    do {
        // process all messages until one needs cooperative processing
        message::Buffer buf;
        message::Message &message = buf;
        bool haveMessage = getNextMessage(buf, false, minPrio);
        int needSync = 0;
        if (haveMessage) {
            if (needsSync(message))
                needSync = 1;
        }
        int anyMessage = boost::mpi::all_reduce(comm(), haveMessage ? 1 : 0, boost::mpi::maximum<int>());
        int anySync = 0;
        if (anyMessage) {
            wasAnyMessage = true;
            anySync = boost::mpi::all_reduce(comm(), needSync, boost::mpi::maximum<int>());
        }

        do {
            if (haveMessage) {
                if (messageReceived)
                    *messageReceived = true;

                if (needsSync(message))
                    needSync = 1;

                MessagePayload pl;
                if (buf.payloadSize() > 0) {
                    pl = Shm::the().getArrayFromName<char>(buf.payloadName());
                    pl.unref();
                }
                quit = handleMessage(&message, pl) ? 0 : 1;
                if (quit) {
                    std::cerr << "Quitting: " << message << std::endl;
                    break;
                }
            }

            if (anySync && !needSync) {
                haveMessage = getNextMessage(buf, true, minPrio);
            }

        } while (anySync && !needSync);

        int anyQuit = boost::mpi::all_reduce(comm(), quit, boost::mpi::maximum<int>());
        if (anyQuit) {
            prepareQuit();
            return false;
        }

        int numMessages = messageBacklog.size() + receiveMessageQueue->getNumMessages();
        maxNumMessages = boost::mpi::all_reduce(comm(), numMessages, boost::mpi::maximum<int>());
        ++numSync;
    } while (!m_replayFinished || (maxNumMessages > 0 && numSync < m_numObjectsPerFrame));

    double start = 0.;
    if (m_benchmark) {
        comm().barrier();
        start = Clock::time();
    }
    bool haveRendered = render();
    if (haveRendered) {
        if (m_benchmark) {
            comm().barrier();
            const double duration = Clock::time() - start;
            if (rank() == 0) {
                sendInfo("render took %f s", duration);
            }
        }
    }

    if (m_maySleep)
        vistle::adaptive_wait(wasAnyMessage || haveRendered, this);

    return true;
}

int Renderer::numTimesteps() const
{
    if (m_objectList.size() <= 1)
        return 0;

    return m_objectList.size() - 1;
}


bool Renderer::addInputObject(int sender, const std::string &senderPort, const std::string &portName,
                              vistle::Object::const_ptr object)
{
    auto it = std::find_if(m_creatorMap.begin(), m_creatorMap.end(), [sender, senderPort](const Creator &c) {
        return c.module == sender && c.port == senderPort;
    });
    if (it != m_creatorMap.end()) {
        if (it->age < object->getExecutionCounter()) {
            //std::cerr << "removing all created by " << creatorId << ", age " << object->getExecutionCounter() << ", was " << it->second.age << std::endl;
            removeAllSentBy(sender, senderPort);
        } else if (it->age == object->getExecutionCounter() && it->iteration < object->getIteration()) {
            std::cerr << "removing all created by " << sender << ":" << senderPort << ", age "
                      << object->getExecutionCounter() << ": new iteration " << object->getIteration() << std::endl;
            removeAllSentBy(sender, senderPort);
        } else if (it->age > object->getExecutionCounter()) {
            std::cerr << "received outdated object created by " << sender << ":" << senderPort << ", age "
                      << object->getExecutionCounter() << ", was " << it->age << std::endl;
            return false;
        } else if (it->age == object->getExecutionCounter() && it->iteration > object->getIteration()) {
            std::cerr << "received outdated object created by " << sender << ":" << senderPort << ", age "
                      << object->getExecutionCounter() << ": old iteration " << object->getIteration() << std::endl;
            return false;
        }
    } else {
        std::string name = getModuleName(object->getCreator());
        it = m_creatorMap.insert(Creator(sender, senderPort, name)).first;
    }
    auto &creator = *it;
    creator.age = object->getExecutionCounter();
    creator.iteration = object->getIteration();

    if (Empty::as(object))
        return true;
    if (auto ph = vistle::PlaceHolder::as(object)) {
        if (ph->originalType() == Empty::type())
            return true;
    }

    auto geo_norm_tex = splitObject(object);

    if (!geo_norm_tex[0]) {
        std::string species = object->getAttribute("_species");
        if (auto tex = vistle::Texture1D::as(object)) {
            auto &cmap = m_colormaps[species];
            cmap.texture = tex;
            cmap.sender = sender;
            cmap.senderPort = senderPort;
            std::cerr << "added colormap " << species << " without object, width=" << tex->getWidth()
                      << ", range=" << tex->getMin() << " to " << tex->getMax() << std::endl;
            return addColorMap(species, tex);
        }
        if (auto ph = vistle::PlaceHolder::as(object)) {
            if (ph->texture()->originalType() == vistle::Texture1D::type())
                return addColorMap(species, ph->texture());
        }
    }

    std::shared_ptr<RenderObject> ro =
        addObjectWrapper(sender, senderPort, object, geo_norm_tex[0], geo_norm_tex[1], geo_norm_tex[2]);
    if (ro) {
        assert(ro->timestep >= -1);
        if (m_objectList.size() <= size_t(ro->timestep + 1))
            m_objectList.resize(ro->timestep + 2);
        m_objectList[ro->timestep + 1].push_back(ro);
    }

#if 1
    std::string variant;
    std::string noobject;
    if (ro) {
        if (!ro->variant.empty())
            variant = " variant " + ro->variant;
    } else {
        noobject = " NO OBJECT generated";
    }
    std::cerr << "++++++Renderer addInputObject " << object->getName() << " type " << object->getType() << " creator "
              << object->getCreator() << " exec " << object->getExecutionCounter() << " iter " << object->getIteration()
              << " block " << object->getBlock() << " timestep " << object->getTimestep() << variant << noobject
              << std::endl;
#endif

    return true;
}

std::shared_ptr<RenderObject> Renderer::addObjectWrapper(int senderId, const std::string &senderPort,
                                                         Object::const_ptr container, Object::const_ptr geom,
                                                         Object::const_ptr normal, Object::const_ptr texture)
{
    auto ro = addObject(senderId, senderPort, container, geom, normal, texture);
    if (ro && !ro->variant.empty()) {
        auto it = m_variants.find(ro->variant);
        if (it == m_variants.end()) {
            it = m_variants.emplace(ro->variant, Variant(ro->variant)).first;
        }
        ++it->second.objectCount;
        if (it->second.visible == RenderObject::DontChange)
            it->second.visible = ro->visibility;
    }
    return ro;
}

void Renderer::removeObjectWrapper(std::shared_ptr<RenderObject> ro)
{
    std::string variant;
    if (ro)
        variant = ro->variant;
    removeObject(ro);
    if (variant.empty()) {
        auto it = m_variants.find(ro->variant);
        if (it != m_variants.end()) {
            --it->second.objectCount;
        }
    }
}

void Renderer::connectionRemoved(const Port *from, const Port *to)
{
    m_geometryCaches.erase(Creator(from->getModuleID(), from->getName()));
    removeAllSentBy(from->getModuleID(), from->getName());

    // connection cut: remove colormap
    auto it = m_colormaps.begin();
    while (it != m_colormaps.end()) {
        auto &cmap = it->second;
        if (cmap.sender == from->getModuleID() && cmap.senderPort == from->getName()) {
            removeColorMap(it->first);
            it = m_colormaps.erase(it);
        } else {
            ++it;
        }
    }
}

void Renderer::removeObject(std::shared_ptr<RenderObject> ro)
{}

void Renderer::removeAllSentBy(int sender, const std::string &senderPort)
{
    auto it = m_geometryCaches.find(Creator(sender, senderPort));
    if (it != m_geometryCaches.end()) {
        it->second->clear();
    }

    for (auto &ol: m_objectList) {
        for (auto &ro: ol) {
            if (ro && ro->senderId == sender && ro->senderPort == senderPort) {
                removeObjectWrapper(ro);
                ro.reset();
            }
        }
        ol.erase(std::remove_if(ol.begin(), ol.end(), [](std::shared_ptr<vistle::RenderObject> ro) { return !ro; }),
                 ol.end());
    }
    while (!m_objectList.empty() && m_objectList.back().empty())
        m_objectList.pop_back();
}

void Renderer::removeAllObjects()
{
    for (auto &ol: m_objectList) {
        for (auto &ro: ol) {
            if (ro) {
                removeObjectWrapper(ro);
                ro.reset();
            }
        }
        ol.clear();
    }
    m_objectList.clear();

    for (auto &cmap: m_colormaps) {
        removeColorMap(cmap.first);
    }
    m_colormaps.clear();
}

const Renderer::VariantMap &Renderer::variants() const
{
    return m_variants;
}

bool Renderer::render()
{
    // no work was done
    return false;
}

bool Renderer::changeParameter(const Parameter *p)
{
    if (p == m_renderMode) {
        switch (m_renderMode->getValue()) {
        case LocalOnly:
        case LocalShmLeader:
            setObjectReceivePolicy(m_fastestObjectReceivePolicy);
            break;
        case MasterOnly:
            setObjectReceivePolicy(m_fastestObjectReceivePolicy >= message::ObjectReceivePolicy::Master
                                       ? m_fastestObjectReceivePolicy
                                       : message::ObjectReceivePolicy::Master);
            break;
        case AllShmLeaders:
        case AllRanks:
            setObjectReceivePolicy(message::ObjectReceivePolicy::Distribute);
            break;
        }
    }

    if (p == m_objectsPerFrame) {
        m_numObjectsPerFrame = m_objectsPerFrame->getValue();
    }

    if (p == m_useGeometryCaches) {
        enableGeometryCaches(m_useGeometryCaches->getValue());
    }

    return Module::changeParameter(p);
}

void Renderer::enableGeometryCaches(bool on)
{
    for (auto &c: m_geometryCaches)
        c.second->enable(on);
}

void Renderer::getBounds(Vector3 &min, Vector3 &max, int t)
{
    if (size_t(t + 1) < m_objectList.size()) {
        for (auto &ro: m_objectList[t + 1]) {
            ro->updateBounds();
            if (!ro->boundsValid()) {
                continue;
            }
            for (int i = 0; i < 3; ++i) {
                min[i] = std::min(min[i], ro->bMin[i]);
                max[i] = std::max(max[i], ro->bMax[i]);
            }
        }
    }

    //std::cerr << "t=" << t << ", bounds min=" << min << ", max=" << max << std::endl;
}

void Renderer::getBounds(Vector3 &min, Vector3 &max)
{
    const Scalar smax = std::numeric_limits<Scalar>::max();
    min = Vector3(smax, smax, smax);
    max = -min;
    for (int t = -1; t < (int)(m_objectList.size()) - 1; ++t)
        getBounds(min, max, t);
}

} // namespace vistle
