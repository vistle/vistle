#include <sstream>
#include <limits>
#include <algorithm>

#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>

#include <vistle/module/module.h>
#include <vistle/core/vec.h>
#include <vistle/core/scalars.h>
#include <vistle/core/message.h>
#include <vistle/core/coords.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/indexed.h>
#include <vistle/util/hostname.h>
#include <vistle/util/affinity.h>

using namespace vistle;

class MpiInfo: public vistle::Module {

 public:
   MpiInfo(const std::string &name, int moduleID, mpi::communicator comm);
   ~MpiInfo();

 private:

   virtual bool compute();
   virtual bool reduce(int timestep);

   bool prepare();
};

using namespace vistle;

MpiInfo::MpiInfo(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("MPI info", name, moduleID, comm)
{
   std::stringstream str;
   str << "ctor: rank " << rank() << "/" << size() << " on host " << hostname() << std::endl;
   sendInfo(str.str());
}

MpiInfo::~MpiInfo() = default;

bool MpiInfo::compute() {

   std::stringstream str;
   str << "compute(): rank " << rank() << "/" << size() << " on host " << hostname() << std::endl;
   str << "Process affinity: " << sched_affinity_map() << std::endl;
   str << "Thread affinity:  " << thread_affinity_map() << std::endl;
   sendInfo(str.str());

   if (rank() == 0) {
       int len = 0;
       char version[MPI_MAX_LIBRARY_VERSION_STRING];
       MPI_Get_library_version(version, &len);
       std::stringstream str;
       str << "MPI version: " << std::string(version, len) << std::endl;
       sendInfo(str.str());
   }

   return true;
}

bool MpiInfo::prepare() {

   return true;
}

bool MpiInfo::reduce(int timestep) {

   return Module::reduce(timestep);
}

MODULE_MAIN(MpiInfo)

