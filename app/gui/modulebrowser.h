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
    ModuleListWidget(QWidget *parent = nullptr);

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

    ModuleBrowser(QWidget *parent = nullptr);
    ~ModuleBrowser();
    bool eventFilter(QObject *object, QEvent *event) override;
    bool handleKeyPress(QKeyEvent *event);
    std::vector<std::pair<int, QString>> getHubs() const;
public slots:
    void addHub(int hub, QString hubName, int nranks, QString address, QString logname, QString realname);
    void removeHub(int hub);
    void addModule(int hub, QString module, QString path, QString description);
    void setFilter(QString filter);

signals:
    void requestRemoveHub(int hub);
    void startModule(int hub, QString moduleName, Qt::Key direction);

private:
    struct SelectedModule {
        bool exists = false;
        std::map<int, QTreeWidgetItem *>::iterator hostIter;
        int moduleIndex = 0;
    } currentModule;

    std::map<int, QTreeWidgetItem *> hubItems;
    bool filterInFocus = false;
    QLineEdit *filterEdit() const;
    Ui::ModuleBrowser *ui;
    bool goToNextModule();
    bool goToPreviousModule();
    void selectModule(Qt::Key dir);
    void initCurrentModule(Qt::Key dir);
    void setCurrentModuleSelected(bool select);

    void prepareMenu(const QPoint &pos);
};

} // namespace gui
#endif
