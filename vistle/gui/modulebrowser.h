#ifndef MODULEBROWSER_H
#define MODULEBROWSER_H

#include <QDockWidget>
#include <QStringList>
#include <QTreeWidget>

class QMimeData;
class QTreeWidgetItem;

namespace gui {

namespace Ui {
class ModuleBrowser;

} //namespace Ui

class ModuleListWidget: public QTreeWidget {
   Q_OBJECT

public:
   ModuleListWidget(QWidget *parent=nullptr);

public slots:
   void setFilter(QString filter);
   void filterItem(QTreeWidgetItem *item) const;

protected:
   QMimeData *mimeData(const QList<QTreeWidgetItem *> dragList) const;

   QString m_filter;
};

class ModuleBrowser: public QWidget {
   Q_OBJECT

public:
   static const char *mimeFormat();
   static int nameRole();
   static int hubRole();
   static int pathRole();

   ModuleBrowser(QWidget *parent=nullptr);
   ~ModuleBrowser();

public slots:
   void addModule(int hub, QString hubName, QString module, QString path);
   void setFilter(QString filter);

private:
   std::map<int, QTreeWidgetItem *> hubItems;


   QLineEdit *filterEdit() const;

   Ui::ModuleBrowser *ui;
};

} // namespace gui
#endif
