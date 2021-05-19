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
   bool eventFilter(QObject *object, QEvent *event) override;
   void keyPressEvent(QKeyEvent *event) override;
   public slots:
   void addModule(int hub, QString hubName, QString module, QString path);
   void setFilter(QString filter);
signals:
   void startModule(int hub, QString moduleName);

   private:

struct SelectedModule{
   bool exists = false;
   std::map<int, QTreeWidgetItem *>::iterator hostIter;
   int moduleIndex = 0;
} currentModule;

std::map<int, QTreeWidgetItem *>
    hubItems;
   bool filterInFocus = false;
   QLineEdit *filterEdit() const;
   void selectNewModule(bool up);

Ui::ModuleBrowser *ui;
void selectNextModule();
void selectPreviousModule();
void selectFirstModule(bool first);
void setCurrentModuleSelected(bool select);
};

} // namespace gui
#endif
