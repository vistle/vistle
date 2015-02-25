#include "sleep.h"
#include <map>
#include <iostream>

#ifdef _WIN32
#else
#include <unistd.h>
#endif

namespace vistle {

bool adaptive_wait(bool work, const void *client) {

   static std::map<const void *, long> idleMap;
   const long Sec = 1000000; // 1 s
#if 1
   const long MinDelay = Sec/1000;
   const long MaxDelay = Sec/100;
#else
   const long MinDelay = 0; // 1 ms
   const long MaxDelay = 0;
#endif

   auto it = idleMap.find(client);
   if (it == idleMap.end())
      it = idleMap.insert(std::make_pair(client, 0)).first;

   auto &idle = it->second;

   if (work) {
      idle = 0;
      return false;
   }

   
   long delay = (float(idle)/Sec)*(float(idle)/Sec)*Sec;
   if (delay < MinDelay)
      delay = MinDelay;
   if (delay > MaxDelay)
      delay = MaxDelay;

   idle += delay;

   if (idle > delay) {
      if (delay < Sec) {
         //std::cerr << "usleep " << delay << std::endl;
         usleep(delay);
      } else {
         sleep(delay/Sec);
         //std::cerr << "sleep " << delay/Sec << std::endl;
      }
   }

   return true;
}

}
