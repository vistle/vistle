#include <module/module.h>
#include <core/unstr.h>
#include <core/celltree.h>

using namespace vistle;

class CreateCelltree: public vistle::Module {

 public:
   CreateCelltree(const std::string &shmname, int rank, int size, int moduleID);
   ~CreateCelltree();

 private:
   virtual bool compute();
};

using namespace vistle;

CreateCelltree::CreateCelltree(const std::string &shmname, int rank, int size, int moduleID)
: Module("CreateCelltree", shmname, rank, size, moduleID)
{

   Port *din = createInputPort("grid_in", "input grid", Port::MULTI);
   Port *dout = createOutputPort("grid_out", "output grid", Port::MULTI);
   din->link(dout);
}

CreateCelltree::~CreateCelltree() {

}

bool CreateCelltree::compute() {

   while(Object::const_ptr grid = takeFirstObject("grid_in")) {

      if (UnstructuredGrid::const_ptr unstr = UnstructuredGrid::as(grid)) {
         unstr->getCelltree();
      }

      passThroughObject("grid_out", grid);
   }

   return true;
}

MODULE_MAIN(CreateCelltree)

