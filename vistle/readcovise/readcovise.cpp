#include <sstream>
#include <iomanip>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "object.h"
#include "readcovise.h"

MODULE_MAIN(ReadCovise)

ReadCovise::ReadCovise(int rank, int size, int moduleID)
   : Module("ReadCovise", rank, size, moduleID) {

   createOutputPort("grid_out");
}

void swap_int(unsigned int * data, const unsigned int num) {

   for (unsigned int i = 0; i < num; i++) {
      *data = (((*data) & 0xff000000) >> 24)
         | (((*data) & 0x00ff0000) >>  8)
         | (((*data) & 0x0000ff00) <<  8)
         | (((*data) & 0x000000ff) << 24);
      data++;
   }
}

void swap_float(float * d, const unsigned int num) {

   unsigned int *data = (unsigned int *) d;
   for (unsigned int i = 0; i < num; i++) {
      *data = (((*data) & 0xff000000) >> 24)
         | (((*data) & 0x00ff0000) >>  8)
         | (((*data) & 0x0000ff00) <<  8)
         | (((*data) & 0x000000ff) << 24);
      data++;
   }
}

ReadCovise::~ReadCovise() {

}

void ReadCovise::readAttributes(const int fd) {

   unsigned int size, num;
   read(fd, &size, sizeof(int));
   swap_int(&size, 1);
   size -= sizeof(int);
   read(fd, &num, sizeof(int));
   swap_int(&num, 1);
   printf("size %d: %d attributes\n", size, num);
   lseek(fd, size, SEEK_CUR);
}

void ReadCovise::readSETELE(const int fd,
                            std::vector<vistle::Object *> & objects,
                            bool byteswap) {
   
   int num;
   read(fd, &num, sizeof(int));
}

void ReadCovise::readUNSGRD(const int fd,
                            std::vector<vistle::Object *> & objects,
                            bool byteswap) {

   unsigned int numElem;
   unsigned int numConn;
   unsigned int numVert;
   read(fd, &numElem, sizeof(int));
   read(fd, &numConn, sizeof(int));
   read(fd, &numVert, sizeof(int));

   if (byteswap) {
      swap_int(&numElem, 1);
      swap_int(&numConn, 1);
      swap_int(&numVert, 1);
   }

   vistle::UnstructuredGrid *usg =
      vistle::UnstructuredGrid::create(numElem, numConn, numVert);

   unsigned int *el = new unsigned int[numElem];
   unsigned int *tl = new unsigned int[numElem];
   unsigned int *cl = new unsigned int[numConn];

   read(fd, el, numElem * sizeof(int));
   read(fd, tl, numElem * sizeof(int));
   read(fd, cl, numConn * sizeof(int));

   if (byteswap) {
      swap_int(el, numElem);
      swap_int(tl, numElem);
      swap_int(cl, numConn);
   }

   for (unsigned int index = 0; index < numElem; index ++) {
      usg->el[index] = el[index];
      usg->tl[index] = (vistle::UnstructuredGrid::Type) tl[index];
   }
   for (unsigned int index = 0; index < numConn; index ++)
      usg->cl[index] = cl[index];
   
   read(fd, &(usg->x[0]), numVert * sizeof(float));
   read(fd, &(usg->y[0]), numVert * sizeof(float));
   read(fd, &(usg->z[0]), numVert * sizeof(float));

   if (byteswap) {
      swap_float(&(usg->x[0]), numVert);
      swap_float(&(usg->y[0]), numVert);
      swap_float(&(usg->z[0]), numVert);
   }

   readAttributes(fd);

   delete[] el;
   delete[] tl;
   delete[] cl;
   
   objects.push_back(usg);
}

void ReadCovise::load(const std::string & name,
                      std::vector<vistle::Object *> & objects) {

   bool byteswap = true;

   int fd = open(name.c_str(), O_RDONLY);
   char buf[7];
   buf[6] = 0;

   int r = read(fd, buf, 6);
   if (!strncmp(buf, "COV_BE", 6))
      byteswap = true;
   else if (!strncmp(buf, "COV_LE", 6))
      byteswap = false;
   else
      lseek(fd, 0, SEEK_SET);

   r = read(fd, buf, 6);
   while (r > 0) {

      if (!strncmp(buf, "SETELE", 6))
         readSETELE(fd, objects, byteswap);

      else if (!strncmp(buf, "UNSGRD", 6))
         readUNSGRD(fd, objects, byteswap);

      r = read(fd, buf, 6);
   }

   close(fd);  
}

bool ReadCovise::compute() {

   std::vector<vistle::Object *> objects;
   load("/home/hpcflon/covise/example-data/tutorial/tiny_geo.covise", objects);

   std::vector<vistle::Object *>::iterator i;
   for (i = objects.begin(); i != objects.end(); i++)
      addObject("grid_out", *i);

   return true;
}

/*
int main(int argc, char **argv) {

   ReadCovise r;
   r.compute();
}

class ReadCovise {
   
public:
   ReadCovise();
   ~ReadCovise();

   void readSETELE(const int fd);
   void readUNSGRD(const int fd);
   void readAttributes(const int fd);

   void load(const std::string & name);
   bool compute();
};

ReadCovise::ReadCovise() {

}
*/
