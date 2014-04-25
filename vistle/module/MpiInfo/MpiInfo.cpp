#include <sstream>
#include <limits>
#include <algorithm>

#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>

#include <module/module.h>
#include <core/vec.h>
#include <core/scalars.h>
#include <core/message.h>
#include <core/coords.h>
#include <core/lines.h>
#include <core/triangles.h>
#include <core/indexed.h>

using namespace vistle;

class MpiInfo: public vistle::Module {

 public:
   MpiInfo(const std::string &shmname, int rank, int size, int moduleID);
   ~MpiInfo();

 private:

   virtual bool compute();
   virtual bool reduce(int timestep);

   bool prepare();
};

using namespace vistle;

MpiInfo::MpiInfo(const std::string &shmname, int rank, int size, int moduleID)
   : Module("MPI info", shmname, rank, size, moduleID)
{
}

MpiInfo::~MpiInfo() {

}

bool MpiInfo::compute() {

   std::vector<char> hostname(1024);
   gethostname(hostname.data(), hostname.size());

   std::stringstream str;
   str << "rank " << rank() << "/" << size() << " on host " << hostname.data() << std::endl;
   sendInfo(str.str());

   return true;
}

bool MpiInfo::prepare() {

   return true;
}

bool MpiInfo::reduce(int timestep) {

   return Module::reduce(timestep);
}

MODULE_MAIN(MpiInfo)

