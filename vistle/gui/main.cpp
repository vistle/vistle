/*********************************************************************************/
/*! \file main.cpp
 *
 * Sets host and port information, initializes main event thread and secondary UI
 * thread.
 */
/**********************************************************************************/
#include "mainwindow.h"
#include "uicontroller.h"
#include <QApplication>

#include <userinterface/userinterface.h>
#include <userinterface/vistleconnection.h>
#include <userinterface/pythonembed.h>
#include <userinterface/pythonmodule.h>
#include <boost/ref.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/uuid/uuid.hpp>

Q_DECLARE_METATYPE(boost::uuids::uuid);

using namespace gui;

int main(int argc, char *argv[])
{
   qRegisterMetaType<Module::Status>("Module::Status");
   qRegisterMetaType<Port::Type>("Port::Type");
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

        vistle::PythonInterface python("Vistle GUI");
        std::cerr << "trying to connect UI to " << host << ":" << port << std::endl;
        QApplication a(argc, argv);
        MainWindow w;
        VistleObserver observer;
        w.setVistleobserver(&observer);
        vistle::UserInterface ui(host, port, &observer);
        ui.registerObserver(&observer);
        vistle::VistleConnection conn(ui);
        vistle::PythonModule pythonmodule(&conn);
        boost::thread runnerThread(boost::ref(conn));
        UiController(&conn, &observer);

        w.setVistleConnection(&conn);
        w.show();
        int val = a.exec();

        conn.cancel();
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
