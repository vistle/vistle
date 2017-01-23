#include <module/module.h>
#include <core/points.h>

using namespace vistle;

class AttributeToVariants: public vistle::Module {

 public:
   AttributeToVariants(const std::string &shmname, const std::string &name, int moduleID);
   ~AttributeToVariants();

 private:
   virtual bool compute();

   StringParameter *p_attribute;
};

using namespace vistle;

AttributeToVariants::AttributeToVariants(const std::string &shmname, const std::string &name, int moduleID)
: Module("copy attribute to variant", shmname, name, moduleID)
{

   Port *din = createInputPort("data_in", "input data", Port::MULTI);
   Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
   din->link(dout);

   p_attribute = addStringParameter("attribute", "name of attribute to copy to variant", "_part");
}

AttributeToVariants::~AttributeToVariants() {

}

bool AttributeToVariants::compute() {

   //std::cerr << "AttributeToVariants: compute: execcount=" << m_executionCount << std::endl;

   auto attr = p_attribute->getValue();
   if (attr.empty())
      return true;

   Object::const_ptr obj = expect<Object>("data_in");
   if (!obj)
      return true;

   auto variant = obj->getAttribute(attr);
   if (variant.empty()) {
      passThroughObject("data_out", obj);
   } else {
      Object::ptr out = obj->clone();
      out->addAttribute("_variant", variant);
      out->addAttribute("_plugin", "Variant");
      addObject("data_out", out);
   }

   return true;
}

MODULE_MAIN(AttributeToVariants)

