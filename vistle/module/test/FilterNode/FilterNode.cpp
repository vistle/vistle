#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/vec.h>

#include "FilterNode.h"
#include <util/enum.h>

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Choices, (X)(Y)(Z)(AbsoluteValue))

FilterNode::FilterNode(const std::string &shmname, const std::string &name, int moduleID)
   : Module("FilterNode", shmname, name, moduleID) {

   createInputPort("data_in");
   createOutputPort("data_out");

   m_nodeParam = addIntParameter("node_num", "Node number", 0);
   setParameterRange(m_nodeParam, (Integer)0, (Integer)size()-1);
   m_invertParam = addIntParameter("invert", "Invert node selection", 0, Parameter::Boolean);
}

FilterNode::~FilterNode() {
}

bool FilterNode::compute() {

   Object::const_ptr data = expect<Object>("data_in");
   if (!data)
      return true;

   const bool invert = m_invertParam->getValue();
   const int node = m_nodeParam->getValue();

   bool pass = node == rank();
   if (invert)
      pass = !pass;

   if (pass) {
      passThroughObject("data_out", data);
   } else {
      Object::ptr obj = data->createEmpty();
      obj->setMeta(data->meta());
      obj->copyAttributes(data);
      addObject("data_out", obj);
   }

   return true;
}

MODULE_MAIN(FilterNode)
