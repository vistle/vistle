#include <module/module.h>
#include <core/unstr.h>
#include <core/vertexownerlist.h>
#include <ctime>

using namespace vistle;

class CreateVertexOwnerList: public vistle::Module {

 public:
   CreateVertexOwnerList(const std::string &shmname, const std::string &name, int moduleID);
   ~CreateVertexOwnerList();

 private:
   virtual bool compute();
};

using namespace vistle;

CreateVertexOwnerList::CreateVertexOwnerList(const std::string &shmname, const std::string &name, int moduleID)
: Module("CreateVertexOwnerList", shmname, name, moduleID)
{

   Port *din = createInputPort("grid_in", "input grid", Port::MULTI);
   Port *dout = createOutputPort("grid_out", "output grid", Port::MULTI);
   din->link(dout);
}

CreateVertexOwnerList::~CreateVertexOwnerList() {

}

bool CreateVertexOwnerList::compute() {
   UnstructuredGrid::const_ptr unstr = expect<UnstructuredGrid>("grid_in");
   if (!unstr)
      return false;

   unstr->getVertexOwnerList();
   passThroughObject("grid_out", unstr);
   return true;
}

MODULE_MAIN(CreateVertexOwnerList)
