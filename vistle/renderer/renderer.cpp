#include <core/message.h>
#include <core/messagequeue.h>
#include <core/object.h>
#include <core/placeholder.h>
#include <core/coords.h>
#include <core/archives.h>
#include <core/archive_loader.h>
#include <core/archive_saver.h>

#include "renderer.h"

#include <util/vecstreambuf.h>
#include <util/sleep.h>
#include <util/stopwatch.h>

namespace mpi = boost::mpi;

namespace vistle {

const int MaxObjectsPerFrame = 50;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(RenderMode,
(LocalOnly)
(MasterOnly)
(AllNodes)
)

Renderer::Renderer(const std::string &description, const std::string &shmname,
                   const std::string &name, const int moduleID)
   : Module(description, shmname, name, moduleID)
   , m_fastestObjectReceivePolicy(message::ObjectReceivePolicy::Local)
{

   setSchedulingPolicy(message::SchedulingPolicy::Ignore); // compute does not have to be called at all
   setReducePolicy(message::ReducePolicy::Never); // because of COMBINE port
   createInputPort("data_in", "input data", Port::COMBINE);

   m_renderMode = addIntParameter("render_mode", "Render on which nodes?", LocalOnly, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_renderMode, RenderMode);

   //std::cerr << "Renderer starting: rank=" << rank << std::endl;
}

Renderer::~Renderer() {

}

static bool needsSync(const message::Message &m) {

   switch (m.type()) {
      case vistle::message::CANCELEXECUTE:
      case vistle::message::OBJECTRECEIVED:
      case vistle::message::QUIT:
      case vistle::message::KILL:
      case vistle::message::ADDPARAMETER:
      case vistle::message::SETPARAMETER:
      case vistle::message::REMOVEPARAMETER:
         return true;
      default:
         return false;
   }
}

bool Renderer::dispatch() {

   message::Buffer buf;
   message::Message &message = buf;

   int quit = 0;
   bool checkAgain = false;
   int numSync = 0;
   do {
      bool haveMessage = false;
      if (messageBacklog.empty()) {
          haveMessage = receiveMessageQueue->tryReceive(message);
      } else {
          buf = messageBacklog.front();
          messageBacklog.pop_front();
          haveMessage = true;
      }

      int sync = 0, allsync = 0;

      if (haveMessage) {
         if (needsSync(message))
            sync = 1;
      }

      MPI_Allreduce(&sync, &allsync, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

      vistle::adaptive_wait(haveMessage || allsync);

      do {
         if (haveMessage) {

            switch (message.type()) {
               case vistle::message::ADDOBJECT: {
                  if (size() == 1 || objectReceivePolicy()==message::ObjectReceivePolicy::Local) {
                     auto &add = static_cast<const message::AddObject &>(message);
                     addInputObject(add.senderId(), add.getSenderPort(), add.getDestPort(), add.takeObject());
                  }
                  break;
               }
               case vistle::message::OBJECTRECEIVED: {
                  vassert(objectReceivePolicy() != message::ObjectReceivePolicy::Local);
                  if (size() > 1) {
                     auto &recv = static_cast<const message::ObjectReceived &>(message);
                     handle(recv);
                  }
                  break;
               }
               default:
                  quit = !handleMessage(&message);
                  if (quit) {
                      std::cerr << "Quitting: " << message << std::endl;
                  }
                  break;
            }

            if (needsSync(message))
               sync = 1;
         }

         if (allsync && !sync) {
            if (messageBacklog.empty()) {
                receiveMessageQueue->receive(message);
            } else {
                 buf = messageBacklog.front();
                 messageBacklog.pop_front();
            }

            haveMessage = true;
         }

      } while(allsync && !sync);

      int numMessages = messageBacklog.size() + receiveMessageQueue->getNumMessages();
      int maxNumMessages = 0;
      MPI_Allreduce(&numMessages, &maxNumMessages, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
      ++numSync;
      checkAgain = maxNumMessages>0 && numSync<MaxObjectsPerFrame;

   } while (checkAgain && !quit);

   int doQuit = 0;
   MPI_Allreduce(&quit, &doQuit, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
   if (doQuit) {
      prepareQuit();
      vistle::message::ModuleExit m;
      sendMessageQueue->send(m);
   } else {
      double start = 0.;
      if (m_benchmark) {
         comm().barrier();
         start = Clock::time();
      }
      if (render() && m_benchmark) {
         comm().barrier();
         const double duration = Clock::time() - start;
         if (rank() == 0) {
             sendInfo("render took %f s", duration);
         }
      }
   }

   return !quit;
}


bool Renderer::addInputObject(int sender, const std::string &senderPort, const std::string & portName,
                                 vistle::Object::const_ptr object) {

   int creatorId = object->getCreator();
   CreatorMap::iterator it = m_creatorMap.find(creatorId);
   if (it != m_creatorMap.end()) {
      if (it->second.age < object->getExecutionCounter()) {
         //std::cerr << "removing all created by " << creatorId << ", age " << object->getExecutionCounter() << ", was " << it->second.age << std::endl;
         removeAllCreatedBy(creatorId);
      } else if (it->second.age == object->getExecutionCounter() && it->second.iter < object->getIteration()) {
         std::cerr << "removing all created by " << creatorId << ", age " << object->getExecutionCounter() << ": new iteration " << object->getIteration() << std::endl;
         removeAllCreatedBy(creatorId);
      } else if (it->second.age > object->getExecutionCounter()) {
         std::cerr << "received outdated object created by " << creatorId << ", age " << object->getExecutionCounter() << ", was " << it->second.age << std::endl;
         return false;
      } else if (it->second.age == object->getExecutionCounter() && it->second.iter > object->getIteration()) {
         std::cerr << "received outdated object created by " << creatorId << ", age " << object->getExecutionCounter() << ": old iteration " << object->getIteration() << std::endl;
         return false;
      }
   } else {
      std::string name = getModuleName(object->getCreator());
      it = m_creatorMap.insert(std::make_pair(creatorId, Creator(object->getCreator(), name))).first;
   }
   Creator &creator = it->second;
   creator.age = object->getExecutionCounter();
   creator.iter = object->getIteration();

   std::shared_ptr<RenderObject> ro;
#if 1
   std::cout << "++++++Renderer addInputObject " << object->getName()
             << " type " << object->getType()
             << " creator " << object->getCreator()
             << " exec " << object->getExecutionCounter()
             << " iter " << object->getIteration()
             << " block " << object->getBlock()
             << " timestep " << object->getTimestep() << std::endl;
#endif

   if (auto tex = vistle::Texture1D::as(object)) {
       if (auto grid = vistle::Coords::as(tex->grid())) {
         ro = addObjectWrapper(sender, senderPort, object, grid, grid->normals(), tex);
       }
   } else if (auto grid = vistle::Coords::as(object)) {
       ro = addObjectWrapper(sender, senderPort, object, grid, grid->normals(), nullptr);
   } else if (auto data = vistle::DataBase::as(object)) {
       if (auto grid = vistle::Coords::as(data->grid())) {
         ro = addObjectWrapper(sender, senderPort, object, grid, grid->normals(), nullptr);
       }
   }
   if (!ro) {
      ro = addObjectWrapper(sender, senderPort, object, object, vistle::Object::ptr(), vistle::Object::ptr());
   }

   if (ro) {
      vassert(ro->timestep >= -1);
      if (m_objectList.size() <= size_t(ro->timestep+1))
         m_objectList.resize(ro->timestep+2);
      m_objectList[ro->timestep+1].push_back(ro);
   }

   return true;
}

std::shared_ptr<RenderObject> Renderer::addObjectWrapper(int senderId, const std::string &senderPort, Object::const_ptr container, Object::const_ptr geom, Object::const_ptr normal, Object::const_ptr texture) {

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

void Renderer::removeObjectWrapper(std::shared_ptr<RenderObject> ro) {

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

void Renderer::connectionRemoved(const Port *from, const Port *to) {

   removeAllSentBy(from->getModuleID(), from->getName());
}

void Renderer::removeObject(std::shared_ptr<RenderObject> ro) {
}

void Renderer::removeAllCreatedBy(int creatorId) {

   for (auto &ol: m_objectList) {
      for (auto &ro: ol) {
         if (ro && ro->container && ro->container->getCreator() == creatorId) {
            removeObjectWrapper(ro);
            ro.reset();
         }
      }
      ol.erase(std::remove_if(ol.begin(), ol.end(), [](std::shared_ptr<vistle::RenderObject> ro) { return !ro; }), ol.end());
   }
   while (!m_objectList.empty() && m_objectList.back().empty())
      m_objectList.pop_back();
}

void Renderer::removeAllSentBy(int sender, const std::string &senderPort) {

   for (auto &ol: m_objectList) {
      for (auto &ro: ol) {
         if (ro && ro->senderId == sender && ro->senderPort == senderPort) {
            removeObjectWrapper(ro);
            ro.reset();
         }
      }
      ol.erase(std::remove_if(ol.begin(), ol.end(), [](std::shared_ptr<vistle::RenderObject> ro) { return !ro; }), ol.end());
   }
   while (!m_objectList.empty() && m_objectList.back().empty())
      m_objectList.pop_back();
}

void Renderer::removeAllObjects() {
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
}

const Renderer::VariantMap &Renderer::variants() const {

    return m_variants;
}

bool Renderer::compute() {
    return true;
}

bool Renderer::handle(const message::ObjectReceived &recv) {

    Object::const_ptr obj, placeholder;
    if (recv.rank() == rank()) {
        obj = Shm::the().getObjectFromName(recv.objectName());
        auto ph = std::make_shared<PlaceHolder>(recv.objectName(), recv.meta(), recv.objectType());
        ph->copyAttributes(obj);
        placeholder = ph;
        assert(obj);
        assert(placeholder);
    }
    RenderMode rm = static_cast<RenderMode>(m_renderMode->getValue());
    if (rm == AllNodes) {
        broadcastObject(obj, recv.rank());
        assert(obj);
    } else {
        broadcastObject(placeholder, recv.rank());
        assert(placeholder);
        if (rm == MasterOnly) {
            if (rank() == 0) {
                if (recv.rank() != 0)
                    obj = receiveObject(recv.rank());
            } else if (rank() == recv.rank()) {
                sendObject(obj, 0);
            }
        }
    }
    if (rm == AllNodes || (rm == MasterOnly && rank() == 0) || rank() == recv.rank())
        addInputObject(recv.senderId(), recv.getSenderPort(), recv.getDestPort(), obj);
    else
        addInputObject(recv.senderId(), recv.getSenderPort(), recv.getDestPort(), placeholder);

    return true;
}

bool Renderer::changeParameter(const Parameter *p) {
    if (p == m_renderMode) {
        switch(m_renderMode->getValue()) {
        case LocalOnly:
            setObjectReceivePolicy(m_fastestObjectReceivePolicy);
            break;
        case MasterOnly:
            setObjectReceivePolicy(message::ObjectReceivePolicy::Master);
            break;
        case AllNodes:
            setObjectReceivePolicy(message::ObjectReceivePolicy::Distribute);
            break;
        }
    }
    return true;
}

void Renderer::getBounds(Vector &min, Vector &max, int t) {

   if (size_t(t+1) < m_objectList.size()) {
      for (auto &ro: m_objectList[t+1]) {
         for (int c=0; c<3; ++c) {
            if (ro->bMin[c] < min[c])
               min[c] = ro->bMin[c];
            if (ro->bMax[c] > max[c])
               max[c] = ro->bMax[c];
         }
      }
   }
}

void Renderer::getBounds(Vector &min, Vector &max) {

   const Scalar smax = std::numeric_limits<Scalar>::max();
   min = Vector3(smax, smax, smax);
   max = -min;
   for (int t=-1; t<(int)(m_objectList.size())-1; ++t)
      getBounds(min, max, t);
}

} // namespace vistle
