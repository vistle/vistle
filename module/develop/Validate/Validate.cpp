#include <vistle/core/object.h>
#include <vistle/core/unstr.h>
#include <vistle/core/message.h>
#include "Validate.h"

MODULE_MAIN(Validate)

using namespace vistle;

Validate::Validate(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("data_in", "data");
}

Validate::~Validate()
{}

bool Validate::compute()
{
    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    if (!obj->check(std::cerr, false)) {
        std::string n = obj->getName();
        sendError("validation failed for %s", n.c_str());
    }

    return true;
}
