#include <boost/interprocess/exceptions.hpp>

// cover
#include <cover/coVRPluginSupport.h>
#include <cover/coCommandLine.h>
#include <cover/OpenCOVER.h>

#include "OsgRenderer.h"

// vistle
#include <util/exception.h>

#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>

using namespace opencover;
using namespace vistle;

class VistlePlugin: public opencover::coVRPlugin, public vrui::coMenuListener {

 public:
   VistlePlugin();
   ~VistlePlugin();
   bool init() override;
   bool destroy() override;
   void menuEvent(vrui::coMenuItem *item) override;
   void preFrame() override;
   void requestQuit(bool killSession) override;
   bool executeAll() override;

 private:
   OsgRenderer *m_module;
   vrui::coButtonMenuItem *executeButton;
};

using opencover::coCommandLine;

VistlePlugin::VistlePlugin()
: m_module(nullptr)
{
   int initialized = 0;
   MPI_Initialized(&initialized);
   if (!initialized) {
      std::cerr << "VistlePlugin: no MPI support - started from within Vistle?" << std::endl;
      //throw(vistle::exception("no MPI support"));
      return;
   }

   if (coCommandLine::argc() < 3) {
      for (int i=0; i<coCommandLine::argc(); ++i) {
         std::cout << i << ": " << coCommandLine::argv(i) << std::endl;
      }
      throw(vistle::exception("at least 2 command line arguments required"));
   }

   int rank, size;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   const std::string &shmname = coCommandLine::argv(1);
   const std::string &name = coCommandLine::argv(2);
   int moduleID = atoi(coCommandLine::argv(3));

   vistle::Module::setup(shmname, moduleID, rank);
   m_module = new OsgRenderer(shmname, name, moduleID);
   m_module->setPlugin(this);
}

VistlePlugin::~VistlePlugin() {

   if (m_module) {
      MPI_Barrier(MPI_COMM_WORLD);
      m_module->prepareQuit();
      delete m_module;
      m_module = nullptr;
   }
}

bool VistlePlugin::init() {

   std::string visMenu = getName();
   VRMenu *covise = VRPinboard::instance()->namedMenu(visMenu.c_str());
   if (!covise)
   {
      VRPinboard::instance()->addMenu(visMenu.c_str(), VRPinboard::instance()->mainMenu->getCoMenu());
      covise = VRPinboard::instance()->namedMenu(visMenu.c_str());
      cover->addSubmenuButton((visMenu+"...").c_str(), NULL, visMenu.c_str(), false, NULL, -1, this);
   }
   vrui::coMenu *coviseMenu = covise->getCoMenu();
   executeButton = new vrui::coButtonMenuItem("Execute");
   executeButton->setMenuListener(this);
   coviseMenu->add(executeButton);

   return m_module;
}

bool VistlePlugin::destroy() {

   if (m_module) {
      MPI_Barrier(MPI_COMM_WORLD);
      m_module->prepareQuit();
      delete m_module;
      m_module = nullptr;
   }

    return true;
}

void VistlePlugin::menuEvent(vrui::coMenuItem *item) {

   if (item == executeButton) {
      executeAll();
   }
}

void VistlePlugin::preFrame() {

   MPI_Barrier(MPI_COMM_WORLD);
   try {
       if (m_module && !m_module->dispatch()) {
           std::cerr << "Vistle requested COVER to quit" << std::endl;
           OpenCOVER::instance()->quitCallback(NULL,NULL);
       }
   } catch (boost::interprocess::interprocess_exception &e) {
       std::cerr << "Module::dispatch: interprocess_exception: " << e.what() << std::endl;
       std::cerr << "   error: code: " << e.get_error_code() << ", native: " << e.get_native_error() << std::endl;
       throw(e);
   } catch (std::exception &e) {
       std::cerr << "Module::dispatch: std::exception: " << e.what() << std::endl;
       throw(e);
   }
}

void VistlePlugin::requestQuit(bool killSession)
{
   MPI_Barrier(MPI_COMM_WORLD);
   m_module->prepareQuit();
   delete m_module;
   m_module = nullptr;
}

bool VistlePlugin::executeAll() {

   return m_module->executeAll();
}

COVERPLUGIN(VistlePlugin);
