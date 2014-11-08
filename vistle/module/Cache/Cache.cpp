#include <boost/lexical_cast.hpp>
#include <module/module.h>

using namespace vistle;

static const int NumPorts = 5;

class Cache: public vistle::Module {

 public:
   Cache(const std::string &shmname, const std::string &name, int moduleID);
   ~Cache();

 private:
   virtual bool compute() override;
};

using namespace vistle;

Cache::Cache(const std::string &shmname, const std::string &name, int moduleID)
: Module("cache input objects", shmname, name, moduleID)
{
   setDefaultCacheMode(ObjectCache::CacheAll);

   for (int i=0; i<NumPorts; ++i) {
      std::string suffix = boost::lexical_cast<std::string>(i);

      createInputPort("data_in"+suffix, "input data "+suffix);
      createOutputPort("data_out"+suffix, "output data "+suffix);
   }
}

Cache::~Cache() {

}

bool Cache::compute() {

   for (;;) {

      int numConnected=0, numObject=0;
      for (int i=0; i<NumPorts; ++i) {
         std::string suffix = boost::lexical_cast<std::string>(i);
         std::string in = std::string("data_in")+suffix;

         if (!isConnected(in))
            continue;
         ++numConnected;

         if (hasObject(in))
            ++numObject;
      }

      if (numObject == 0 || numObject < numConnected) {
         break;
      }

      for (int i=0; i<NumPorts; ++i) {
         std::string suffix = boost::lexical_cast<std::string>(i);
         std::string in = std::string("data_in")+suffix;
         std::string out = std::string("data_out")+suffix;

         if (!isConnected(in))
            continue;

         Object::const_ptr obj = takeFirstObject(in);
         passThroughObject(out, obj);
      }
   }

   return true;
}

MODULE_MAIN(Cache)

