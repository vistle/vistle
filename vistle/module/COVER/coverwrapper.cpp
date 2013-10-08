/* wrapper for COVER to distribute environment variables to slaves before
 * initialization of static OpenSceneGraph objects happens */

#include <mpi.h>
#ifndef _WIN32
#include <dlfcn.h>
#endif
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include <util/findself.h>

#ifndef RENAME_MAIN
#error "RENAME_MAIN not defined"
#endif

#define xstr(s) #s
#define str(s) xstr(s)

int main(int argc, char *argv[]) {

   const char libcover[] = "libmpicover.so";
   const char mainname[] = str(RENAME_MAIN);

   std::string bindir = vistle::getbindir(argc, argv);

   MPI_Init(&argc, &argv);

   int rank = -1;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   std::map<std::string, std::string> env;
   std::map<std::string, bool> envToSet;
   if (rank == 0) {
      for (auto v: {
           "PATH",
           "LD_LIBRARY_PATH",
           "DYLD_LIBRARY_PATH",
           "DYLD_FRAMEWORK_PATH",
           "LANG",
           "COCONFIG",
           "COCONFIG_LOCAL",
           "COCONFIG_DEBUG",
           "COVISE_CONFIG",
           //"COVISE_HOST",
           "COVISEDIR",
           "COVISE_PATH",
           "ARCHSUFFIX",
           "OSGFILEPATH",
           "OSG_FILE_PATH",
           "OSG_NOTIFY_LEVEL",
           "OSG_LIBRARY_PATH",
           "OSG_LD_LIBRARY_PATH",
   }) {
         const char *val = getenv(v);
         if (val)
            env[v] = val;
      }

      std::string covisedir = env["COVISEDIR"];
      std::string archsuffix = env["ARCHSUFFIX"];

      if (!covisedir.empty()) {
         if (FILE *fp = popen((covisedir+"/bin/print_covise_env").c_str(), "r")) {
            std::vector<char> buf(10000);
            while (fgets(buf.data(), buf.size(), fp)) {
               auto sep = std::find(buf.begin(), buf.end(), '=');
               if (sep != buf.end()) {
                  std::string name = std::string(buf.begin(), sep);
                  ++sep;
                  auto end = std::find(sep, buf.end(), '\n');
                  std::string val = std::string(sep, end);
                  //std::cerr << name << "=" << val << std::endl;
                  env[name] = val;
               }
               //ld_library_path = buf.data();
               //std::cerr << "read ld_lib: " << ld_library_path << std::endl;
            }
            pclose(fp);
         }
      }
   }

   std::string vistleplugin = bindir + "/../../lib/OpenCOVER/plugins/libVistlePlugin";
#ifdef __APPLE__
   vistleplugin += ".dylib";
#else
   vistleplugin += ".so";
#endif
   env["VISTLE_PLUGIN"] = vistleplugin;
   //std::cerr << "Vistle plugin: " << vistleplugin << std::endl;

   std::string ldpath, dyldpath, dyldfwpath, covisepath;

   int numvars = env.size();
   MPI_Bcast(&numvars, 1, MPI_INT, 0, MPI_COMM_WORLD);
   auto it = env.begin();
   for (int i=0; i<numvars; ++i) {
      std::string name;
      std::string value;
      if (rank == 0) {
         name = it->first;
         value = it->second;
      }

      auto sync_string = [rank](std::string &s) {
         std::vector<char> buf;
         int len = -1;
         if (rank == 0)
            len = s.length()+1;
         MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
         buf.resize(len);
         if (rank == 0)
            strcpy(buf.data(), s.c_str());
         MPI_Bcast(buf.data(), buf.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
         s = buf.data();
      };
      sync_string(name);
      sync_string(value);

#if 0
      if (name == "COVISE_PATH") {
         // adapt in order to find VistlePlugin
         covisepath = bindir + "/../..";
         if (!value.empty()) {
            covisepath += ":";
            covisepath += value;
         }
         value = covisepath;
      }
#endif

      setenv(name.c_str(), value.c_str(), 1 /* overwrite */);

      if (rank == 0)
         ++it;
      else
         env[name] = value;

      //std::cerr << name << " -> " << value << std::endl;
   }

   typedef int (*main_t)(int, char *[]);
   main_t realmain = NULL;
   int ret = 0;
#if 0
   std::string abslib = bindir + "/../../lib/" + libcover;
#else
   std::string coviselibdir = env["COVISEDIR"] + "/" + env["ARCHSUFFIX"] + "/lib/";
   std::string abslib = coviselibdir + libcover;
#endif
   void *handle = dlopen(abslib.c_str(), RTLD_LAZY);
   if (!handle) {
      std::cerr << "failed to dlopen " << abslib << ": " << dlerror() << std::endl;
      ret = 1;
      goto finish;
   }

   realmain = (main_t)dlsym(handle, mainname);
   if (!realmain) {
      std::cerr << "could not find " << mainname << " in " << libcover << std::endl;
      ret = 1;
      goto finish;
   }

   ret = realmain(argc, argv);

finish:
   if (handle)
      dlclose(handle);
   MPI_Finalize();

   return ret;
}
