/*********************************************************************************/
/*! \file main.cpp
 *
 * Sets host and port information, initializes main event thread and secondary UI
 * thread.
 */
/**********************************************************************************/
#include "mainwindow.h"
#include <QApplication>

#include "handler.h"
#include "consts.h"

#include <userinterface/userinterface.h>
#include <boost/ref.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

using namespace gui;

int main(int argc, char *argv[])
{
   qRegisterMetaType<ModuleStatus>("ModuleStatus");
   qRegisterMetaType<ItemType>("ItemType");
   qRegisterMetaType<GraphicsType>("GraphicsType");

    try {
        std::string host = "localhost";
        unsigned short port = 8193;

        if (argc > 1) {
            host = argv[1];
        }
        if (argc > 2) {
            host = atoi(argv[2]);
        }

        QApplication a(argc, argv);
        MainWindow w;

        vistle::UserInterface ui(host, port);
        VistleObserver *printer = new VistleObserver();
        ui.registerObserver(printer);
        UiRunner *runner = new UiRunner(ui);
        boost::thread runnerThread(boost::ref(*runner));

        w.setVistleobserver(printer);
        w.setUiRunner(runner);
        w.show();
        int val = a.exec();

        runner->cancel();
        runnerThread.join();

        return val;

    } catch(std::exception &ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "unknown exception" << std::endl;
        return 1;
    }
}
