/*********************************************************************************/
/*! \file main.cpp
 *
 * Sets host and port information, initializes main event thread and secondary UI
 * thread.
 */
/**********************************************************************************/
#include "mainwindow.h"
#include <QApplication>

#include "vistleconnection.h"
#include "consts.h"

#include <userinterface/userinterface.h>
#include <boost/ref.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/uuid/uuid.hpp>

Q_DECLARE_METATYPE(boost::uuids::uuid);

using namespace gui;

int main(int argc, char *argv[])
{
   qRegisterMetaType<ModuleStatus>("ModuleStatus");
   qRegisterMetaType<ItemType>("ItemType");
   qRegisterMetaType<GraphicsType>("GraphicsType");
   qRegisterMetaType<boost::uuids::uuid>();

    try {
        std::string host = "localhost";
        unsigned short port = 8193;

        if (argc > 1) {
            host = argv[1];
        }
        if (argc > 2) {
            port = atoi(argv[2]);
        }

        QApplication a(argc, argv);
        MainWindow w;

        std::cerr << "trying to connect UI to " << host << ":" << port << std::endl;
        vistle::UserInterface ui(host, port);
        VistleObserver *printer = new VistleObserver();
        ui.registerObserver(printer);
        VistleConnection *runner = new VistleConnection(ui);
        boost::thread runnerThread(boost::ref(*runner));

        w.setVistleobserver(printer);
        w.setVistleConnection(runner);
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
