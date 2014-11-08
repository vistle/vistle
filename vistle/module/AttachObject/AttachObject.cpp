#include <module/module.h>
#include <core/points.h>

using namespace vistle;

class AttachObject: public vistle::Module {

 public:
   AttachObject(const std::string &shmname, const std::string &name, int moduleID);
   ~AttachObject();

 private:
   virtual bool compute();
};

using namespace vistle;

AttachObject::AttachObject(const std::string &shmname, const std::string &name, int moduleID)
: Module("AttachObject", shmname, name, moduleID)
{

   Port *din = createInputPort("data_in", "input data", Port::MULTI);
   Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
   din->link(dout);
}

AttachObject::~AttachObject() {

}

bool AttachObject::compute() {

   //std::cerr << "AttachObject: compute: execcount=" << m_executionCount << std::endl;

   while(Object::const_ptr obj = takeFirstObject("data_in")) {

      //Object::ptr out = obj->clone();
      Object::ptr out = boost::const_pointer_cast<Object>(obj);
      Object::ptr att(new Points(4));
      obj->addAttachment("test", att);

      addObject("data_out", out);
   }

   return true;
}

MODULE_MAIN(AttachObject)

