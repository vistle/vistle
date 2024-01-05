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

    friend class ModuleBrowser; // for calling keyPressEvent

public:
    ModuleListWidget(QWidget *parent = nullptr);

public slots:
    void setFilter(QString filter);
    bool filterModule(QTreeWidgetItem *item) const;

signals:
    void showStatusTip(const QString &tip);

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMimeData *mimeData(const QList<QTreeWidgetItem *> &dragList) const override;
#else
    QMimeData *mimeData(const QList<QTreeWidgetItem *> dragList) const override;
#endif

    QString m_filter;
};

class ModuleBrowser: public QWidget {
    Q_OBJECT

public:
    enum WidgetIndex {
        Main = 0,
        Filtered,
        NumWidgets,
        Visible,
    };

    static const char *mimeFormat();
    static int nameRole();
    static int hubRole();
    static int pathRole();
    static int categoryRole();
    static int toolTipRole();
    static int descriptionRole();

    ModuleBrowser(QWidget *parent = nullptr);
    ~ModuleBrowser();
    bool eventFilter(QObject *object, QEvent *event) override;
    bool handleKeyPress(QKeyEvent *event);
    std::vector<std::pair<int, QString>> getHubs() const;
    QTreeWidgetItem *getHubItem(int hub, WidgetIndex idx = Main) const;
    QTreeWidgetItem *getCategoryItem(int hub, QString category, WidgetIndex idx = Main) const;

public slots:
    void addHub(int hub, QString hubName, int nranks, QString address, int port, QString logname, QString realname,
                bool hasUi, QString systype, QString arch, QString info);
    void removeHub(int hub);
    void addModule(int hub, QString module, QString path, QString category, QString description);
    void setFilter(QString filter);

signals:
    void requestRemoveHub(int hub);
    void requestAttachDebugger(int hub);
    void startModule(int hub, QString moduleName, Qt::Key direction);
    void showStatusTip(const QString &tip);

private:
    WidgetIndex visibleWidgetIndex() const;
    ModuleListWidget *moduleWidget(WidgetIndex idx = Main) const;
    QTreeWidgetItem *addCategory(int hub, QString category, QString description, WidgetIndex idx);

    QString m_filter;
    ModuleListWidget *m_moduleListWidget[NumWidgets];
    std::map<int, QTreeWidgetItem *> m_hubItems[NumWidgets];
    bool m_filterInFocus = false;
    QLineEdit *filterEdit() const;
    Ui::ModuleBrowser *ui = nullptr;

    QPixmap m_iconCluster, m_iconServer, m_iconClusterUi, m_iconServerUi;

    void prepareMenu(const QPoint &pos);
    void writeSettings();
    void readSettings();
    int m_primaryHub;
    std::map<std::string, bool> m_categoryExpanded;
};

} // namespace gui
#endif
