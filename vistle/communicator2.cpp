/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <list>
#include <vector>

int acceptClient() {

   int server = socket(AF_INET, SOCK_STREAM, 0);
   int reuse = 1;
   setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
   
   struct sockaddr_in serv, addr;
   serv.sin_family = AF_INET;
   serv.sin_addr.s_addr = INADDR_ANY;
   serv.sin_port = htons(8192);
   
   if (bind(server, (struct sockaddr *) &serv, sizeof(serv)) < 0) {
      perror("bind error");
      exit(1);
   }
   listen(server, 0);
   
   socklen_t len = sizeof(addr);
   int client = accept(server, (struct sockaddr *) &addr, &len);
   
   close(server);
   return client;
}


int main(int argc, char **argv) {

   int moduleID = 0;
   int rank = 0;
   char *buf = new char[64];

#ifdef HAVE_MPI
   int size;
   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif

#ifdef HAVE_MPI
   // post requests for MPI messages
   char *rbuf = new char[64];
   memset(buf, 0, 64);

   std::vector<MPI_Request> request;
   MPI_Request r;
   MPI_Irecv(rbuf, 64, MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &r);
   request.push_back(r);
   MPI_Barrier(MPI_COMM_WORLD);
#endif

   int client = 0;

   if (rank == 0)
      client = acceptClient();

   bool done = false;

   while (!done) {

      if (rank == 0) {

         // poll socket
         fd_set set;
         FD_ZERO(&set);
         FD_SET(client, &set);
         
         struct timeval t = { 0, 0 };

         select(client + 1, &set, 0, 0, &t);
         
         if (FD_ISSET(client, &set)) {
            read(client, buf, 1);

#ifdef HAVE_MPI
            if (buf[0] != '\r' && buf[0] != '\n') {
               for (int index = 0; index < size; index ++)
                  if (index != rank)
                     MPI_Send(buf, 64, MPI_BYTE, index, 0, MPI_COMM_WORLD);
            }
#endif

            if (buf[0] == 'q')
               done = true;

            if (buf[0] == 's') {
               moduleID++;
#ifdef HAVE_MPI
               MPI_Comm interComm;
               MPI_Comm_spawn("module", MPI_ARGV_NULL, size, MPI_INFO_NULL, 0,
                              MPI_COMM_WORLD, &interComm, MPI_ERRCODES_IGNORE);
#else
               int pid = fork();
               if (pid == 0) {
                  execve("module", 0, 0);
               }
#endif               
            }
         }
      }

#ifdef HAVE_MPI
      // poll MPI
      int flag;
      int index;
      MPI_Status status;

      MPI_Testany(request.size(), &(request[0]), &index, &flag, &status);
      
      if (flag) {
         
         // message from rank 0
         if (status.MPI_TAG == 0) {
            printf("[%02d] message from [%02d]: [%s]\n", rank, index, rbuf);
            if (rbuf[0] == 'q')
               done = true;

            if (rbuf[0] == 's') {
               moduleID++;
               MPI_Comm interComm;
               MPI_Comm_spawn("module", MPI_ARGV_NULL, size, MPI_INFO_NULL, 0,
                              MPI_COMM_WORLD, &interComm, MPI_ERRCODES_IGNORE);
            }
            
            MPI_Request r;
            MPI_Irecv(rbuf, 64, MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &r);
            request[index] = r;
         }
      }
#endif
      usleep(1000);
   }

   if (rank == 0)
      close(client);

#ifdef HAVE_MPI
   MPI_Finalize();
#endif
   return 0;
}
