#include <vistle/core/object.h>
#include <vistle/core/unstr.h>
#include <vistle/core/message.h>
#include "PrintAttributes.h"

MODULE_MAIN(PrintAttributes)

using namespace vistle;

PrintAttributes::PrintAttributes(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    createInputPort("data_in");
}

PrintAttributes::~PrintAttributes()
{}

bool PrintAttributes::compute()
{
    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    print(obj);
    if (auto db = DataBase::as(obj)) {
        if (auto grid = db->grid()) {
            sendInfo("grid attributes begin");
            print(grid);
            sendInfo("grid attributes end");
        }
    }

    return true;
}

void PrintAttributes::print(Object::const_ptr obj)
{
    std::stringstream str;
    int t = obj->getTimestep();
    auto attribs = obj->getAttributeList();
    str << attribs.size() << " attributes for " << obj->getName() << " (t = " << t << ")";
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
