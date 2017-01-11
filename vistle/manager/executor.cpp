/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#else
#include <process.h>
#define NOMINMAX
#include<Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#endif
#include <util/sysdep.h>

#include <mpi.h>

#include <sys/types.h>

#include <cstdlib>
#include <cstdio>

#include <sstream>
#include <iostream>
#include <iomanip>

#include <core/message.h>
#include <core/messagequeue.h>
#include <core/object.h>
#include <core/shm.h>

#include "communicator.h"
#include "executor.h"

using namespace boost::interprocess;

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
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

   MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &m_size);

   // process with the smallest rank on each host allocates shm
   const int HOSTNAMESIZE = 256;

   char hostname[HOSTNAMESIZE];
   std::vector<char> hostnames(HOSTNAMESIZE * m_size);
   gethostname(hostname, HOSTNAMESIZE - 1);

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

   MPI_Allgather(hostname, HOSTNAMESIZE, MPI_CHAR,
                 &hostnames[0], HOSTNAMESIZE, MPI_CHAR, MPI_COMM_WORLD);

   bool first = true;
   for (int index = 0; index < m_rank; index ++)
      if (!strncmp(hostname, &hostnames[index * HOSTNAMESIZE], HOSTNAMESIZE))
         first = false;

   std::vector<std::string> hosts;
   for (int index=0; index<m_size; ++index) {
      hosts.push_back(&hostnames[HOSTNAMESIZE*index]);
   }

   if (first) {
      shared_memory_object::remove(m_name.c_str());
      vistle::Shm::create(m_name, 0, m_rank, NULL);
   }
   MPI_Barrier(MPI_COMM_WORLD);

   char name[HOSTNAMESIZE+30];
   if (!m_rank)
      strncpy(name, m_name.c_str(), m_name.length()+1);
   MPI_Bcast(name, sizeof(name), MPI_CHAR, 0, MPI_COMM_WORLD);
   if (m_rank) {
      m_name = name;
   }

   if (!first)
      vistle::Shm::attach(m_name, 0, m_rank, NULL);
   MPI_Barrier(MPI_COMM_WORLD);

   m_comm = new vistle::Communicator(m_rank, hosts);
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
