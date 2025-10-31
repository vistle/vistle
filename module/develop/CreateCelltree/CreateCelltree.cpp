#include <vistle/module/module.h>
#include <vistle/core/unstr.h>
#include <vistle/core/celltree.h>

using namespace vistle;

class CreateCelltree: public vistle::Module {
public:
    CreateCelltree(const std::string &name, int moduleID, mpi::communicator comm);
    ~CreateCelltree();

private:
    bool compute() override;
};

using namespace vistle;

CreateCelltree::CreateCelltree(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    Port *din = createInputPort("grid_in", "input grid", Port::MULTI);
    Port *dout = createOutputPort("grid_out", "output grid", Port::MULTI);
    din->link(dout);
}

CreateCelltree::~CreateCelltree()
{}

bool CreateCelltree::compute()
{
    Object::const_ptr obj = expect<Object>("grid_in");
    if (!obj)
        return true;
    auto cti = obj->getInterface<CelltreeInterface<3>>();
    if (!cti) {
        sendError("CelltreeInterface is required on grid_in");
        return true;
    }

    cti->getCelltree();

    auto nobj = obj->clone();
    updateMeta(nobj);
    addObject("grid_out", nobj);

    return true;
}

MODULE_MAIN(CreateCelltree)
