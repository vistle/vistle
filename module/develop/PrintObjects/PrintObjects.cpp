#include <vistle/core/object.h>
#include <vistle/core/unstr.h>
#include <vistle/core/message.h>
#include <vistle/util/enum.h>
#include <vistle/util/coRestraint.h>
#include "PrintObjects.h"

MODULE_MAIN(PrintObjects)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Mode, (Attributes)(MetaData)(Data))

PrintObjects::PrintObjects(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("data_in", "data");
    m_mode = addIntParameter("mode", "print mode", Attributes, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_mode, Mode);

    m_blocks = addStringParameter("blocks", "block ranges", "all", Parameter::Restraint);
    m_timesteps = addStringParameter("timesteps", "timestep ranges", "all", Parameter::Restraint);
    m_iterations = addStringParameter("iterations", "iterations ranges", "all", Parameter::Restraint);
}

PrintObjects::~PrintObjects()
{}

bool PrintObjects::compute()
{
    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    auto matches = [&](const std::string &s, int value) {
        coRestraint restr;
        restr.add(s);
        if (restr(value)) {
            return true;
        }
        return false;
    };

    auto b = getBlock(obj);
    if (!matches(m_blocks->getValue(), b)) {
        return true;
    }

    auto t = getTimestep(obj);
    if (!matches(m_timesteps->getValue(), t)) {
        return true;
    }

    auto i = getIteration(obj);
    if (!matches(m_iterations->getValue(), i)) {
        return true;
    }

    if (m_mode->getValue() == Attributes) {
        print(obj);
        if (auto db = DataBase::as(obj)) {
            if (auto grid = db->grid()) {
                sendInfo("grid attributes begin");
                print(grid);
                sendInfo("grid attributes end");
            }
        }
    } else {
        bool verbose = m_mode->getValue() == Data;
        std::stringstream str;
        obj->print(str, verbose);
        sendInfo(str.str());
    }

    return true;
}

void PrintObjects::print(Object::const_ptr obj)
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
