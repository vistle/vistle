#include "hub.h"

#include <iostream>

int main(int argc, char *argv[]) {

   try {
      vistle::Hub hub;
      if (!hub.init(argc, argv)) {
         return 1;
      }

      while(hub.dispatch())
         ;
   } catch (vistle::exception &e) {
      std::cerr << "Hub: fatal exception: " << e.what() << std::endl << e.where() << std::endl;
      return 1;
   } catch (std::exception &e) {
      std::cerr << "Hub: fatal exception: " << e.what() << std::endl;
      return 1;
   }

   return 0;
}
