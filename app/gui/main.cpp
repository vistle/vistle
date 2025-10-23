/*********************************************************************************/
/*! \file main.cpp
 *
 * Sets host and port information, initializes main event thread and secondary UI
 * thread.
 */
/**********************************************************************************/
#include "uicontroller.h"
#include <vistle/util/exception.h>
#include <vistle/core/uuid.h>
#include <QApplication>
#include <QIcon>

#include <iostream>
#include <array>

#include "macosutils.h"
#include "colormap.h"

Q_DECLARE_METATYPE(boost::uuids::uuid)

using namespace gui;

void debugMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    std::cerr << message.toStdString() << std::endl;
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    macos_disable_tabs();
#endif

    qRegisterMetaType<Module::Status>("Module::Status");
    qRegisterMetaType<Port::Type>("Port::Type");
    qRegisterMetaType<gui::Range>("Range");
    qRegisterMetaType<boost::uuids::uuid>();

    try {
        QApplication a(argc, argv);

        QCoreApplication::setOrganizationName("HLRS");
        QCoreApplication::setOrganizationDomain("hlrs.de");
        QCoreApplication::setApplicationName("Vistle");

        // we want decimal points rather than commas
        QLocale::setDefault(QLocale::c());

        a.setAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
        a.setAttribute(Qt::AA_DontShowIconsInMenus);
        //DebugBreak();

        //std::cerr << "installing debug msg handler" << std::endl;
        qInstallMessageHandler(debugMessageHandler);
        QIcon icon(":/icons/vistle.png");
        a.setWindowIcon(icon);
        UiController control(argc, argv, &a);
        if (control.init()) {
            int val = a.exec();
            control.finish();
            return val;
        }
        return 1;
    } catch (vistle::except::exception &ex) {
        std::cerr << "GUI: fatal exception: " << ex.what() << std::endl << ex.where() << std::endl;
        return 1;
    } catch (std::exception &ex) {
        std::cerr << "GUI: fatal exception: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "GUI: fatal exception: unknown" << std::endl;
        return 1;
    }
}
