#ifndef MODULEBROWSER_H
#define MODULEBROWSER_H

#include <QDockWidget>
#include <QStringList>
#include <QListWidget>

class QMimeData;
class QListWidgetItem;

namespace gui {

namespace Ui {
class ModuleBrowser;

} //namespace Ui

class ModuleListWidget: public QListWidget {

public:
   ModuleListWidget(QWidget *parent=nullptr);

protected:
   QMimeData *mimeData(const QList<QListWidgetItem *> dragList) const;

};

class ModuleBrowser: public QWidget {
   Q_OBJECT

public:
   static const char *mimeFormat();
   static int nameRole();

   ModuleBrowser(QWidget *parent=nullptr);
   ~ModuleBrowser();

public slots:
   void addModule(QString module);

private:
   Ui::ModuleBrowser *ui;
};

} // namespace gui
#endif
