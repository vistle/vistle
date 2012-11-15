/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#else
#include<Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#define usleep Sleep
#define close closesocket
typedef int socklen_t;
#endif

#include <mpi.h>

#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>

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
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

   MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &m_size);

   // process with the smallest rank on each host allocates shm
   const int HOSTNAMESIZE = 64;

   char hostname[HOSTNAMESIZE];
   std::vector<char> hostnames(HOSTNAMESIZE * m_size);
   gethostname(hostname, HOSTNAMESIZE - 1);

   std::stringstream instanceName;
   if (!m_rank) {
      int pid = static_cast<int>(getpid());
      instanceName << "vistle_" << hostname << "_" << pid;
   }
   m_name = instanceName.str();

   MPI_Allgather(hostname, HOSTNAMESIZE, MPI_CHAR,
                 &hostnames[0], HOSTNAMESIZE, MPI_CHAR, MPI_COMM_WORLD);

   bool first = true;
   for (int index = 0; index < m_rank; index ++)
      if (!strncmp(hostname, &hostnames[index * HOSTNAMESIZE], HOSTNAMESIZE))
         first = false;

   if (first) {
      shared_memory_object::remove(m_name.c_str());
      vistle::Shm::create(m_name, 0, m_rank, NULL);
   }
   MPI_Barrier(MPI_COMM_WORLD);

   char name[HOSTNAMESIZE+30];
   if (!m_rank)
      strncpy(name, m_name.c_str(), m_name.length());
   MPI_Bcast(name, sizeof(name), MPI_CHAR, 0, MPI_COMM_WORLD);
   if (m_rank) {
      m_name = name;
   }

   if (!first)
      vistle::Shm::attach(m_name, 0, m_rank, NULL);
   MPI_Barrier(MPI_COMM_WORLD);

   m_comm = new vistle::Communicator(argc, argv, m_rank, m_size);
}

Executor::~Executor()
{
   delete m_comm;

   shared_memory_object::remove(m_name.c_str());
}

void Executor::registerInterpreter(PythonEmbed *pi) {

   m_comm->registerInterpreter(pi);
}

void Executor::run()
{
   config();

   while (!m_comm->dispatch()) {
      usleep(100);
   }
}

} // namespace vistle
