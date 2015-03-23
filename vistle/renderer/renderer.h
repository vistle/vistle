#ifndef RENDERER_H
#define RENDERER_H

#include <module/module.h>
#include "renderobject.h"
#include "export.h"

namespace vistle {

class V_RENDEREREXPORT Renderer: public Module {

 public:
   Renderer(const std::string &desc, const std::string &shmname,
            const std::string &name, const int moduleID);
   virtual ~Renderer();

   bool dispatch();

 protected:
   virtual boost::shared_ptr<RenderObject> addObject(int senderId, const std::string &senderPort,
         Object::const_ptr container, Object::const_ptr geom, Object::const_ptr normal, Object::const_ptr colors, Object::const_ptr texture) = 0;
   virtual void removeObject(boost::shared_ptr<RenderObject> ro);

 private:
   bool compute() override; // provide dummy implementation of Module::compute

   virtual void render() = 0;

   bool addInputObject(int sender, const std::string &senderPort, const std::string & portName,
         vistle::Object::const_ptr object) override;
   void connectionRemoved(const Port *from, const Port *to) override;

   void removeAllCreatedBy(int creatorId);
   void removeAllSentBy(int sender, const std::string &senderPort);

   struct Creator {
      Creator(int id, const std::string &basename)
      : id(id)
      , age(0)
      {
         std::stringstream s;
         s << basename << "_" << id;
         name = s.str();

      }
      int id;
      int age;
      std::string name;
   };
   typedef std::map<int, Creator> CreatorMap;
   CreatorMap m_creatorMap;

   std::vector<boost::shared_ptr<RenderObject>> m_objectList;
   IntParameter *m_renderMode;
};

} // namespace vistle

#endif
