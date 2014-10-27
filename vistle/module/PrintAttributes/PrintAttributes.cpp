#include <core/object.h>
#include <core/unstr.h>
#include <core/message.h>
#include "PrintAttributes.h"

MODULE_MAIN(PrintAttributes)

using namespace vistle;

PrintAttributes::PrintAttributes(const std::string &shmname, int rank, int size, int moduleID)
    : Module("show object attributes", shmname, rank, size, moduleID) {

    createInputPort("data_in");
}

PrintAttributes::~PrintAttributes() {

}

bool PrintAttributes::compute() {

   while (hasObject("data_in")) {

      Object::const_ptr obj = takeFirstObject("data_in");
      std::stringstream str;
      auto attribs = obj->getAttributeList();
      str << attribs.size() << " attributes for " << obj->getName();
      sendInfo(str.str());
      std::string empty;
      for (auto attr: attribs) {
         std::string val = obj->getAttribute(attr);
         if (val.empty()) {
            empty += " ";
            empty += attr;
         }
      }
      if (!empty.empty()) {
         sendInfo("empty:" + empty);
      }
      for (auto attr: obj->getAttributeList()) {
         std::string val = obj->getAttribute(attr);
         if (!val.empty()) {
            std::stringstream str;
            str << "   " << attr << " -> " << val;
            sendInfo(str.str());
         }
      }
   }

   return true;
}
