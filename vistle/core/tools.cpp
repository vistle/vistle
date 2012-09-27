#include <sstream>
#include <cstdlib>

#ifndef _WIN32
#include <execinfo.h>
#endif

#include "tools.h"

namespace vistle {

std::string backtrace()
{
   std::stringstream str;

#ifndef _WIN32
   const int MaxFrames = 32;

   void* buffer[MaxFrames] = { 0 };
   const int count = ::backtrace(buffer, MaxFrames);

   char** symbols = ::backtrace_symbols(buffer, count);

   if (symbols) {

      for (int n=0; n<count; ++n) {

         str << "   " << symbols[n] << std::endl;
      }

      free(symbols);
   }
#endif

   return str.str();
}

} // namespace vistle
