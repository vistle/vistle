#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/vec.h>

#include "FilterNode.h"
#include <util/enum.h>

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Choices, (Rank)(BlockNumber)(Timestep))

FilterNode::FilterNode(const std::string &shmname, const std::string &name, int moduleID)
   : Module("FilterNode", shmname, name, moduleID) {

   createInputPort("data_in");
   createOutputPort("data_out");

   m_criterionParam = addIntParameter("criterion", "Selection criterion", 0, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_criterionParam, Choices);
   m_nodeParam = addIntParameter("selection", "Number of MPI rank, block, timestep to select", 0);
   setParameterRange(m_nodeParam, (Integer)0, (Integer)size()-1);
   m_invertParam = addIntParameter("invert", "Invert block selection", 0, Parameter::Boolean);
}

FilterNode::~FilterNode() {
}

bool FilterNode::changeParameter(const Parameter *p) {
    if (p == m_criterionParam) {
        switch(m_criterionParam->getValue()) {
        case Rank:
            setParameterRange(m_nodeParam, (Integer)0, (Integer)size()-1);
            break;
        case BlockNumber:
        case Timestep:
        default:
            setParameterRange(m_nodeParam, (Integer)0, (Integer)InvalidIndex);
            break;
        }
    }
    return true;
}

bool FilterNode::compute() {

   Object::const_ptr data = expect<Object>("data_in");
   if (!data)
      return true;

   const bool invert = m_invertParam->getValue();
   const Integer select = m_nodeParam->getValue();

   bool pass = true;
   switch (m_criterionParam->getValue()) {
   case Rank:
       pass = select == rank();
       break;
   case BlockNumber:
       pass = select == data->getBlock();
       break;
   case Timestep:
       pass = select == data->getTimestep();
       break;
   }

   if (invert)
      pass = !pass;

   if (pass) {
      passThroughObject("data_out", data);
   } else {
      Object::ptr obj = data->cloneType();
      obj->setMeta(data->meta());
      obj->copyAttributes(data);
      addObject("data_out", obj);
   }

   return true;
}

MODULE_MAIN(FilterNode)
