/*********************************************************************************/
/*! \file main.cpp
 *
 * Sets host and port information, initializes main event thread and secondary UI
 * thread.
 */
/**********************************************************************************/
#include "uicontroller.h"
#include <userinterface/pythoninterface.h>
#include <util/exception.h>
#include <QApplication>
#include <QIcon>

#include <core/uuid.h>

Q_DECLARE_METATYPE(boost::uuids::uuid);

using namespace gui;

int main(int argc, char *argv[])
{
   qRegisterMetaType<Module::Status>("Module::Status");
   qRegisterMetaType<Port::Type>("Port::Type");
   qRegisterMetaType<boost::uuids::uuid>();

   try {
      vistle::PythonInterface python("Vistle GUI");
      QApplication a(argc, argv);
      QIcon icon(":/vistle.png");
      a.setWindowIcon(icon);
      UiController control(argc, argv, &a);
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
