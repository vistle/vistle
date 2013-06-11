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
#include<Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
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
#include "pythonembed.h"
#include "executor.h"

#ifdef WIN32
#define usleep Sleep
#define close closesocket
typedef int socklen_t;
#endif
using namespace boost::interprocess;

namespace vistle {

using namespace message;

Executor::Executor(int argc, char *argv[])
   : m_name("vistle")
     , m_rank(-1)
     , m_size(-1)
     , m_comm(NULL)
     , m_interpreter(NULL)
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

   m_comm = new vistle::Communicator(argc, argv, m_rank, hosts);
}

Executor::~Executor()
{
   delete m_comm;
   delete m_interpreter;

   shared_memory_object::remove(m_name.c_str());
}

void Executor::setFile(const std::string &filename) {

   m_comm->setFile(filename);
}

void Executor::setInput(const std::string &input) {

   m_comm->setInput(input);
}

bool Executor::config(int argc, char *argv[]) {

   return true;
}

void Executor::run()
{
   if (!config(m_argc, m_argv))
      return;

   while (m_comm->dispatch()) {
      usleep(100);
   }
}

} // namespace vistle
