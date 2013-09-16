/*********************************************************************************/
/*! \file main.cpp
 *
 * Sets host and port information, initializes main event thread and secondary UI
 * thread.
 */
/**********************************************************************************/
#include "uicontroller.h"
#include <QApplication>

#include <boost/uuid/uuid.hpp>

Q_DECLARE_METATYPE(boost::uuids::uuid);

using namespace gui;

int main(int argc, char *argv[])
{
   qRegisterMetaType<Module::Status>("Module::Status");
   qRegisterMetaType<Port::Type>("Port::Type");
   qRegisterMetaType<boost::uuids::uuid>();

   try {
      QApplication a(argc, argv);
      UiController control(argc, argv, &a);
      int val = a.exec();
      control.finish();
      return val;
   } catch(std::exception &ex) {
      std::cerr << "exception: " << ex.what() << std::endl;
      return 1;
   } catch (...) {
      std::cerr << "unknown exception" << std::endl;
      return 1;
   }
}
