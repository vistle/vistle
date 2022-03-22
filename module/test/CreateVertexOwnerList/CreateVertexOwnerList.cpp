#include <vistle/module/module.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vertexownerlist.h>
#include <ctime>

using namespace vistle;

class CreateVertexOwnerList: public vistle::Module {
public:
    CreateVertexOwnerList(const std::string &name, int moduleID, mpi::communicator comm);
    ~CreateVertexOwnerList();

private:
    virtual bool compute();
};

using namespace vistle;

CreateVertexOwnerList::CreateVertexOwnerList(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    Port *din = createInputPort("grid_in", "input grid", Port::MULTI);
    Port *dout = createOutputPort("grid_out", "output grid", Port::MULTI);
    din->link(dout);
}

CreateVertexOwnerList::~CreateVertexOwnerList()
{}

bool CreateVertexOwnerList::compute()
{
    UnstructuredGrid::const_ptr unstr = expect<UnstructuredGrid>("grid_in");
    if (!unstr)
        return false;

    unstr->getVertexOwnerList();
    auto nunstr = unstr->clone();
    updateMeta(nunstr);
    addObject("grid_out", nunstr);
    return true;
}

MODULE_MAIN(CreateVertexOwnerList)
