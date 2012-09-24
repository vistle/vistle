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

#include "message.h"
#include "messagequeue.h"
#include "object.h"

#include "communicator.h"
#include "executor.h"

using namespace boost::interprocess;

namespace vistle {

using namespace message;

Executor::Executor(const std::string &name)
   : m_name(name)
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

   std::string instanceName;

   // process with the smallest rank on each host allocates shm
   const int HOSTNAMESIZE = 64;

   char hostname[HOSTNAMESIZE];
   std::vector<char> hostnames(HOSTNAMESIZE * m_size);
   gethostname(hostname, HOSTNAMESIZE - 1);

#if 0
   if (!m_rank) {
      int pid = static_cast<int>(getpid());
      instanceName << hostname << "_" << pid;
   }
#endif

   MPI_Allgather(hostname, HOSTNAMESIZE, MPI_CHAR,
                 &hostnames[0], HOSTNAMESIZE, MPI_CHAR, MPI_COMM_WORLD);

   bool first = true;
   for (int index = 0; index < m_rank; index ++)
      if (!strncmp(hostname, &hostnames[index * HOSTNAMESIZE], HOSTNAMESIZE))
         first = false;

   if (first) {
      shared_memory_object::remove(m_name.c_str());
      vistle::Shm::instance(0, m_rank, NULL);
   }
   MPI_Barrier(MPI_COMM_WORLD);

   if (!first)
      vistle::Shm::instance(0, m_rank, NULL);
   MPI_Barrier(MPI_COMM_WORLD);

   m_comm = new vistle::Communicator(m_rank, m_size);
}

Executor::~Executor()
{
   delete m_comm;

   shared_memory_object::remove(m_name.c_str());
}

void Executor::run()
{
   config();

   bool done = false;
   while (!done) {
      done = m_comm->dispatch();
      usleep(100);
   }
}

bool Executor::handle(const message::Message &m) {

   return m_comm->handleMessage(m);
}

void Executor::spawn(const int moduleID, const char * name) {

   handle(Spawn(0, m_rank, moduleID, name));
}

void Executor::setParam(const int moduleID, const char * name, const vistle::Scalar value) {

   handle(SetFloatParameter(0, m_rank, moduleID, name, value));
}

void Executor::setParam(const int moduleID, const char * name, const std::string & value) {

   handle(SetFileParameter(0, m_rank, moduleID, name, value));
}

void Executor::setParam(const int moduleID, const char * name, const vistle::Vector & value) {

   handle(SetVectorParameter(0, m_rank, moduleID, name, value));
}

void Executor::connect(const int moduleA, const char * aPort,
             const int moduleB, const char * bPort) {

   handle(Connect(0, m_rank, moduleA, aPort, moduleB, bPort));
}

void Executor::compute(const int moduleID) {

   handle(Compute(0, m_rank, moduleID));
}

} // namespace vistle
