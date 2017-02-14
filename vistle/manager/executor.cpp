/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#ifdef _WIN32
#include <process.h>
#endif

#include <util/sysdep.h>
#include <util/hostname.h>

#include <mpi.h>

#include <sys/types.h>

#include <cstdlib>
#include <cstdio>

#include <sstream>
#include <iostream>
#include <iomanip>

#include <boost/mpi.hpp>

#include <core/message.h>
#include <core/messagequeue.h>
#include <core/object.h>
#include <core/shm.h>

#include "communicator.h"
#include "executor.h"

namespace interprocess = boost::interprocess;
namespace mpi = boost::mpi;

namespace vistle {

using namespace message;

Executor::Executor(int argc, char *argv[])
   : m_name("vistle")
     , m_rank(-1)
     , m_size(-1)
     , m_comm(NULL)
     , m_argc(argc)
     , m_argv(argv)
{
   MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &m_size);


   if (argc < 4) {
      std::cerr << "usage: " << argv[0] << " [hostname] [port] [dataPort]" << std::endl;
      std::cerr << "  hostname and port where Vistle hub can be reached have to be specified" << std::endl;
      exit(1);
   }

   unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);
   m_name = Shm::instanceName(argv[1], port);
   unsigned short dataPort = boost::lexical_cast<unsigned short>(argv[3]);

   if (!m_rank) {
      uint64_t len = m_name.length();
      MPI_Bcast(&len, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
      MPI_Bcast(const_cast<char *>(m_name.data()), len, MPI_BYTE, 0, MPI_COMM_WORLD);
   } else {
      uint64_t len = 0;
      MPI_Bcast(&len, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
      std::vector<char> buf(len);
      MPI_Bcast(buf.data(), len, MPI_BYTE, 0, MPI_COMM_WORLD);
      m_name = std::string(buf.data(), len);
   }

   std::string hostname = vistle::hostname();
   std::vector<std::string> hostnames;
   mpi::all_gather(mpi::communicator(), hostname, hostnames);

   bool first = true;
   for (int index = 0; index < m_rank; index ++)
      if (hostnames[index] == hostname)
         first = false;

   if (first) {
      interprocess::shared_memory_object::remove(m_name.c_str());
      vistle::Shm::create(m_name, 0, m_rank, NULL);
   }
   MPI_Barrier(MPI_COMM_WORLD);

   mpi::broadcast(mpi::communicator(), m_name, 0);

   if (!first)
      vistle::Shm::attach(m_name, 0, m_rank, NULL);
   MPI_Barrier(MPI_COMM_WORLD);

   m_comm = new vistle::Communicator(m_rank, hostnames);
   if (!m_comm->connectHub(argv[1], port, dataPort)) {
      std::stringstream err;
      err << "failed to connect to Vistle hub on " << argv[1] << ":" << port;
      throw vistle::exception(err.str());
   }
}

Executor::~Executor()
{
   delete m_comm;

   Shm::the().detach();
}

void Executor::setModuleDir(const std::string &dir) {

    return m_comm->setModuleDir(dir);
}

bool Executor::config(int argc, char *argv[]) {

   return true;
}

void Executor::run() {

   if (!config(m_argc, m_argv))
      return;

   m_comm->run();
}

} // namespace vistle
