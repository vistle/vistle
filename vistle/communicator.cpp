#include <mpi.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <list>

int main(int argc, char **argv) {

   int rank, size;
   MPI_Init(&argc, &argv);

   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);

   char *buf = new char[64];
   memset(buf, 0, 64);
   char **rbuf = new char*[size];
   MPI_Request *request = new MPI_Request[size];

   // post requests for MPI messages from all nodes
   for (int index = 0; index < size; index ++) {
      rbuf[index] = new char[64];
      MPI_Irecv(rbuf[index], 64, MPI_BYTE, index, 0, MPI_COMM_WORLD,
                &(request[index]));
   }
   
   MPI_Barrier(MPI_COMM_WORLD);
   
   int server = 0;
   int client = 0;

   if (rank == 0) {

      server = socket(AF_INET, SOCK_STREAM, 0);
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
      client = accept(server, (struct sockaddr *) &addr, &len);
   }

   bool done = false;

   std::list<>;

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

            if (buf[0] != '\r' && buf[0] != '\n') {
               for (int index = 0; index < size; index ++)
                  if (index != rank)
                     MPI_Send(buf, 64, MPI_BYTE, index, 0, MPI_COMM_WORLD);
            }

            if (buf[0] == 'q')
               done = true;
            
            if (buf[0] == 's') {
               MPI_Comm intercomm;
               MPI_Comm_spawn("./module",
                              MPI_ARGV_NULL, size, MPI_INFO_NULL,
                              0, MPI_COMM_WORLD, &intercomm,
                              MPI_ERRCODES_IGNORE);
            }
         }
      }
      
      // poll MPI
      int index, flag;
      MPI_Testany(size, request, &index, &flag, MPI_STATUS_IGNORE);
      
      if (flag) {
         MPI_Irecv(rbuf[index], 64, MPI_BYTE, index, 0, MPI_COMM_WORLD,
                   &(request[index]));
         printf("[%02d] message from [%02d]: [%s]\n", rank, index, rbuf[index]);
         if (*rbuf[index] == 'q')
            done = true;

         if (*rbuf[index] == 's') {
            MPI_Comm intercomm;
            MPI_Comm_spawn("./module", MPI_ARGV_NULL,
                           size, MPI_INFO_NULL,
                           0, MPI_COMM_WORLD, &intercomm,
                           MPI_ERRCODES_IGNORE);
         }
      }
      usleep(1000);
   }

   if (rank == 0) {
      close(client);
      close(server);
   }
   MPI_Finalize();
   return 0;
}
