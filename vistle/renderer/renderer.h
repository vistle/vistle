#ifndef RENDERER_H
#define RENDERER_H

#include <module/module.h>
#include <util/enum.h>
#include "renderobject.h"
#include "export.h"

namespace vistle {

DEFINE_ENUM_WITH_STRING_CONVERSIONS(RenderMode,
                                    (LocalOnly)
                                    (MasterOnly)
                                    (AllNodes)
);

class V_RENDEREREXPORT Renderer: public Module {

 public:
   Renderer(const std::string &desc, const std::string &name,
            const int moduleID, mpi::communicator comm);
   virtual ~Renderer();

   bool dispatch(bool *messageReceived=nullptr) override;

   int numTimesteps() const;
   void getBounds(Vector3 &min, Vector3 &max);
   void getBounds(Vector3 &min, Vector3 &max, int time);

   struct Variant {
       friend class boost::serialization::access;

       Variant(const std::string &name=std::string())
       : name(name) {}

       std::string name;
       int objectCount = 0;
       RenderObject::InitialVariantVisibility visible = RenderObject::DontChange;
   };
   typedef std::map<std::string, Variant> VariantMap;
   const VariantMap &variants() const;

   struct ColorMap {
       int creator = vistle::message::Id::Invalid;
       int sender = vistle::message::Id::Invalid;
       std::string senderPort;
       vistle::Texture1D::const_ptr texture;
   };

   typedef std::map<std::string, ColorMap> ColorMapMap;

 protected:
   bool handleMessage(const message::Message *message) override;

   virtual bool addColorMap(const std::string &species, Texture1D::const_ptr texture);
   virtual bool removeColorMap(const std::string &species);

   virtual std::shared_ptr<RenderObject> addObject(int senderId, const std::string &senderPort,
         Object::const_ptr container, Object::const_ptr geom, Object::const_ptr normal, Object::const_ptr texture) = 0;
   virtual void removeObject(std::shared_ptr<RenderObject> ro);

   bool changeParameter(const Parameter *p) override;
   bool compute() override; // provide dummy implementation of Module::compute
   void connectionRemoved(const Port *from, const Port *to) override;

   int m_fastestObjectReceivePolicy;
   void removeAllObjects();

   bool m_maySleep = true;

 private:

   virtual bool render() = 0;

   bool handleAddObject(const message::AddObject &add);

   bool addInputObject(int sender, const std::string &senderPort, const std::string & portName,
         vistle::Object::const_ptr object) override;
   std::shared_ptr<RenderObject> addObjectWrapper(int senderId, const std::string &senderPort,
         Object::const_ptr container, Object::const_ptr geom, Object::const_ptr normal, Object::const_ptr texture);
   void removeObjectWrapper(std::shared_ptr<RenderObject> ro);

   void removeAllCreatedBy(int creatorId);
   void removeAllSentBy(int sender, const std::string &senderPort);

   struct Creator {
      Creator(int id, const std::string &basename)
      : id(id)
      , age(0)
      , iter(-1)
      {
         std::stringstream s;
         s << basename << "_" << id;
         name = s.str();

      }
      int id;
      int age;
      int iter;
      std::string name;
   };
   typedef std::map<int, Creator> CreatorMap;
   CreatorMap m_creatorMap;

   std::vector<std::vector<std::shared_ptr<RenderObject>>> m_objectList;
   IntParameter *m_renderMode;
   bool needsSync(const message::Message &m) const;

   VariantMap m_variants;
   ColorMapMap m_colormaps;
};

} // namespace vistle

#endif
