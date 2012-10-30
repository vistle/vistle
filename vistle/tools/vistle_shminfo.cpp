/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */


#include <iostream>
#include <cstdlib>

#include <mpi.h>

#include <core/object.h>

using namespace vistle;

int main(int argc, char ** argv) {

   MPI_Init(&argc, &argv);

   if (argc < 2) {
      std::cerr << "vistle session id is required" << std::endl;
      exit(1);
   }

   Shm &shm = Shm::attach(argv[1], -1, -1);


   for (size_t i=0; i<Shm::s_shmdebug->size(); ++i) {
      const ShmDebugInfo &info = (*Shm::s_shmdebug)[i];
      std::cerr << (info.type ? info.type : '0') << " " << (info.deleted ? "D" : "A") << " " << info.name;
      if (info.deleted) {
         std::cerr << std::endl;
      } else {
         switch(info.type) {
            case 'O':
               if (info.handle) {
                  boost::shared_ptr<const Object> obj = shm.getObjectFromHandle(info.handle);
                  if (obj) {
                     std::cerr << " type " << obj->getType();
                     std::cerr << " " << obj->refcount() << std::endl;
                  } else {
                     std::cerr << "ERR" << std::endl;
                  }
               } else {
                  std::cerr << "no handle" << std::endl;
               }
               break;
            case 'V':
               if (info.handle) {
                  const void *p = Shm::instance().getShm().get_address_from_handle(info.handle);
                  const ShmVector<int> *v = static_cast<const ShmVector<int> *>(p);
                  std::cerr << " " << v->refcount() << std::endl;
               } else {
                  std::cerr << " no handle" << std::endl;
               }
               break;
            case '\0':
               std::cerr << std::endl;
               break;
            default:
               std::cerr << " unknown type" << std::endl;
               break;
         }
      }
   }

   MPI_Finalize();

   return 0;
}
