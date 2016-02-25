#include <module/module.h>
#include <core/points.h>

using namespace vistle;

class Variant: public vistle::Module {

 public:
   Variant(const std::string &shmname, const std::string &name, int moduleID);
   ~Variant();

 private:
   virtual bool compute();

   StringParameter *p_variant;
};

using namespace vistle;

Variant::Variant(const std::string &shmname, const std::string &name, int moduleID)
: Module("add variant attribute", shmname, name, moduleID)
{

   Port *din = createInputPort("data_in", "input data", Port::MULTI);
   Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
   din->link(dout);

   p_variant = addStringParameter("variant", "variant name", "NULL");
}

Variant::~Variant() {

}

bool Variant::compute() {

   //std::cerr << "Variant: compute: execcount=" << m_executionCount << std::endl;

   auto variant = p_variant->getValue();
   Object::const_ptr obj = expect<Object>("data_in");
   if (!obj)
      return true;

   Object::ptr out = obj->clone();
   out->addAttribute("_variant", variant);
   if (!variant.empty()) {
       out->addAttribute("_plugin", "Variant");
   }
   addObject("data_out", out);

   return true;
}

MODULE_MAIN(Variant)

