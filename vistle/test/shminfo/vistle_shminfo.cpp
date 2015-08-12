/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */


#include <iostream>
#include <fstream>
#include <cstdlib>

#include <mpi.h>

#include <core/shm.h>
#include <core/object.h>

using namespace vistle;

int main(int argc, char ** argv) {

   MPI_Init(&argc, &argv);

   std::string shmid;

   if (argc < 2) {
      std::fstream shmlist;
      shmlist.open(Shm::shmIdFilename().c_str(), std::ios::in);

      while (!shmlist.eof() && !shmlist.fail()) {
         std::string id;
         shmlist >> id;
         if (id.find("vistle_") == 0) {
            shmid = id;
         }
      }

      if (shmid.empty()) {
         std::cerr << "vistle session id is required" << std::endl;
         exit(1);
      } else {
         std::cerr << "defaulting to vistle session id " << shmid << std::endl;
      }
   } else {
      shmid = argv[1];
   }

#ifdef SHMDEBUG
   try {
      Shm::attach(shmid, -1, -1);
   } catch (std::exception &ex) {
      std::cerr << "failed to attach to " << shmid << ": " << ex.what() << std::endl;
      MPI_Finalize();
      exit(1);
   }

   for (size_t i=0; i<Shm::s_shmdebug->size(); ++i) {
      const ShmDebugInfo &info = (*Shm::s_shmdebug)[i];
      std::cout << (info.type ? info.type : '0') << " " << (info.deleted ? "del" : "ref") << " " << info.name;
      if (info.deleted) {
         std::cout << std::endl;
      } else {
         switch(info.type) {
            case 'O':
               if (info.handle) {
                  boost::shared_ptr<const Object> obj = Shm::the().getObjectFromHandle(info.handle);
                  if (obj) {
                     std::cout << " type " << obj->getType();
                     std::cout << " ref " << obj->refcount() << std::endl;
                  } else {
                     std::cout << "ERR" << std::endl;
                  }
               } else {
                  std::cout << "no handle" << std::endl;
               }
               break;
            case 'V':
               if (info.handle) {
                  const void *p = Shm::the().shm().get_address_from_handle(info.handle);
                  const ShmVector<int> *v = static_cast<const ShmVector<int> *>(p);
                  std::cout << " ref " << v->refcount() << std::endl;
               } else {
                  std::cout << " no handle" << std::endl;
               }
               break;
            case '\0':
               std::cout << std::endl;
               break;
            default:
               std::cout << " unknown type" << std::endl;
               break;
         }
      }
   }
#else
   std::cerr << "recompile with -DSHMDEBUG" << std::endl;
#endif

   MPI_Finalize();

   return 0;
}
