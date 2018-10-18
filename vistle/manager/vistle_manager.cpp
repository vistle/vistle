/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <mpi.h>

#include <iostream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <sstream>

#include <util/directory.h>
#include <core/objectmeta.h>
#include <core/object.h>
#include "executor.h"
#include "vistle_manager.h"


#ifdef COVER_ON_MAINTHREAD
#include <thread>
#include <functional>
#include <deque>
#include <condition_variable>

#if defined(HAVE_QT) && defined(MODULE_THREAD)
#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#endif
#endif

using namespace vistle;
namespace dir = vistle::directory;

class Vistle: public Executor {
   public:
   Vistle(int argc, char *argv[]) : Executor(argc, argv) {}
   bool config(int argc, char *argv[]) {

      std::string prefix = dir::prefix(argc, argv);
      setModuleDir(dir::module(prefix));
      return true;
   }
};

#ifdef COVER_ON_MAINTHREAD
static std::mutex main_thread_mutex;
static std::condition_variable main_thread_cv;
static std::deque<std::function<void()>> main_func;
static bool main_done = false;

void run_on_main_thread(std::function<void()> &func) {

    {
       std::unique_lock<std::mutex> lock(main_thread_mutex);
       main_func.emplace_back(func);
    }
    main_thread_cv.notify_all();
    std::unique_lock<std::mutex> lock(main_thread_mutex);
    main_thread_cv.wait(lock, []{ return main_done || main_func.empty(); });
}
#endif

int main(int argc, char ** argv)
{
   int provided = MPI_THREAD_SINGLE;
   MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
   if (provided != MPI_THREAD_MULTIPLE) {
      std::cerr << "insufficient thread support in MPI: MPI_THREAD_MULTIPLE is required (maybe set MPICH_MAX_THREAD_SAFETY=multiple?)" << std::endl;
      exit(1);
   }

#ifdef COVER_ON_MAINTHREAD
#if defined(HAVE_QT) && defined(MODULE_THREAD)
   if (!qApp) {
       std::cerr << "early creation of QApplication object" << std::endl;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
       if (getenv("DISPLAY"))
#endif
       {
           auto app = new QApplication(argc, argv);
       }
       if (qApp) {
           qApp->setAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
           QIcon icon(":/icons/vistle.png");
           qApp->setWindowIcon(icon);
       }
   }
#endif
   auto t = std::thread([argc, argv](){
#endif

   try {
      vistle::registerTypes();
      Vistle(argc, argv).run();
   } catch(vistle::exception &e) {

      std::cerr << "fatal exception: " << e.what() << std::endl << e.where() << std::endl;
      exit(1);
   } catch(std::exception &e) {

      std::cerr << "fatal exception: " << e.what() << std::endl;
      exit(1);
   }

#ifdef COVER_ON_MAINTHREAD
   std::unique_lock<std::mutex> lock(main_thread_mutex);
   main_done = true;
   main_thread_cv.notify_all();
   });


   for (;;) {
      std::unique_lock<std::mutex> lock(main_thread_mutex);
      main_thread_cv.wait(lock, []{ return main_done || !main_func.empty(); });
      if (main_done) {
         //main_thread_cv.notify_all();
         break;
      }

      std::function<void()> f;
      if (!main_func.empty()) {
         std::cerr << "executing on main thread..." << std::endl;
         f = main_func.front();
         if (f)
            f();
         main_func.pop_front();
      }
      lock.unlock();
      main_thread_cv.notify_all();
   }

   t.join();
#endif

   MPI_Finalize();
   return 0;
}
