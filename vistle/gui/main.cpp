/*********************************************************************************/
/*! \file main.cpp
 *
 * Sets host and port information, initializes main event thread and secondary UI
 * thread.
 */
/**********************************************************************************/
#include "uicontroller.h"
#include <util/exception.h>
#include <core/uuid.h>
#include <QApplication>
#include <QIcon>


Q_DECLARE_METATYPE(boost::uuids::uuid);

using namespace gui;

int main(int argc, char *argv[])
{
   qRegisterMetaType<Module::Status>("Module::Status");
   qRegisterMetaType<Port::Type>("Port::Type");
   qRegisterMetaType<boost::uuids::uuid>();

   try {
      QApplication a(argc, argv);
      QIcon icon(":/vistle.png");
      a.setWindowIcon(icon);
      UiController control(argc, argv, &a);
      control.init();
      int val = a.exec();
      control.finish();
      return val;
   } catch (vistle::except::exception &ex) {
      std::cerr << "GUI: fatal exception: " << ex.what() << std::endl << ex.where() << std::endl;
      return 1;
   } catch(std::exception &ex) {
      std::cerr << "GUI: fatal exception: " << ex.what() << std::endl;
      return 1;
   } catch (...) {
      std::cerr << "GUI: fatal exception: unknown" << std::endl;
      return 1;
   }
}
