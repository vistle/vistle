#include "modulebrowser.h"
#include "ui_modulebrowser.h"
#include "module.h"

#include <vistle/core/message.h>

#include <QDebug>
#include <QKeyEvent>
#include <QMimeData>
#include <QMenu>
#include <QSettings>
#include <fstream>

namespace gui {

enum ItemTypes {
    Hub,
    Category,
    Module,
};

static std::array<ModuleBrowser::WidgetIndex, 2> ModuleLists{ModuleBrowser::Main, ModuleBrowser::Filtered};
static std::array<const char *, 10> Categories{"Simulation", "Read",        "Filter",  "Map",    "Geometry",
                                               "Render",     "Information", "General", "UniViz", "Test"};
static std::map<std::string, std::string> CategoryDescriptions{
    {"Simulation", "acquire data from running simulations for in situ processing"},
    {"Read", "read data from files"},
    {"Filter", "transform abstract data into abstract data"},
    {"Map", "map abstract data to geometric shapes"},
    {"Geometry", "process geometric shapes"},
    {"Render", "render images from geometric shapes"},
    {"General", "process data at any stage in the pipeline"},
    {"Information", "provide information on input"},
    {"UniViz", "flow visualization modules by Filip Sadlo"},
    {"Test", "support testing and development of Vistle and modules"},
};

class CategoryItem: public QTreeWidgetItem {
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem &other) const override
    {
        if (other.type() != Category)
            return *static_cast<const QTreeWidgetItem *>(this) < other;
        std::string mytext = text(0).toStdString();
        std::string otext = other.text(0).toStdString();
        auto myit = std::find(Categories.begin(), Categories.end(), mytext);
        auto oit = std::find(Categories.begin(), Categories.end(), otext);
        if (myit == oit) {
            return mytext < otext;
        }
        if (myit == Categories.end())
            return false;
        return myit < oit;
    }
};


ModuleListWidget::ModuleListWidget(QWidget *parent): QTreeWidget(parent)
{
    header()->setVisible(false);
    setSelectionMode(SingleSelection);
    sortItems(0, Qt::AscendingOrder);
    connect(this, &QTreeWidget::currentItemChanged, [this](QTreeWidgetItem *cur, QTreeWidgetItem *prev) {
        if (prev) {
            prev->setSelected(false);
        }
        if (cur) {
            cur->setSelected(true);
            emit showStatusTip(cur->data(0, ModuleBrowser::descriptionRole()).toString());
        }
    });
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QMimeData *ModuleListWidget::mimeData(const QList<QTreeWidgetItem *> &dragList) const
#else
QMimeData *ModuleListWidget::mimeData(const QList<QTreeWidgetItem *> dragList) const
#endif
{
    if (dragList.empty())
        return nullptr;

    QMimeData *md = QTreeWidget::mimeData(dragList);

    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    for (QTreeWidgetItem *item: dragList) {
        if (item->type() != Module)
            continue;
        stream << item->data(0, ModuleBrowser::hubRole()).toInt();
        stream << item->text(0);
    }
    md->setData(ModuleBrowser::mimeFormat(), encodedData);
    return md;
}

void ModuleListWidget::setFilter(QString filter)
{
    m_filter = filter.trimmed();

    bool firstMatch = true;
    for (int idx = 0; idx < topLevelItemCount(); ++idx) {
        auto ti = topLevelItem(idx);
        ti->setExpanded(true);
        for (int i = 0; i < ti->childCount(); ++i) {
            auto cat = ti->child(i);
            cat->setExpanded(true);
            unsigned nvis = 0;
            for (int j = 0; j < cat->childCount(); ++j) {
                auto item = cat->child(j);
                if (filterModule(item)) {
                    ++nvis;
                    item->setHidden(false);
                    item->setSelected(firstMatch);
                    if (firstMatch)
                        setCurrentItem(item);
                    firstMatch = false;
                } else {
                    item->setSelected(false);
                    item->setHidden(true);
                }
            }
            cat->setHidden(nvis == 0);
        }
    }
}

bool ModuleListWidget::filterModule(QTreeWidgetItem *item) const
{
    assert(item->type() == Module);

    bool onlyModules = false;
    auto matchType = Qt::CaseInsensitive;
    for (const auto &c: m_filter) {
        if (c.isUpper()) {
            matchType = Qt::CaseSensitive;
            onlyModules = true;
        }
    }

    const auto name = item->data(0, ModuleBrowser::nameRole()).toString();
    bool match = name.contains(m_filter, matchType);
    if (!match && !onlyModules) {
        const auto desc = item->data(0, ModuleBrowser::descriptionRole()).toString();
        match = desc.contains(m_filter, matchType);
    }
#if 0
    if (!match && !onlyModules) {
        const auto cat = item->data(0, ModuleBrowser::categoryRole()).toString();
        match = cat.contains(m_filter, matchType);
    }
#endif

    return match;
}

const char *ModuleBrowser::mimeFormat()
{
    return "application/x-modulebrowser";
}

int ModuleBrowser::nameRole()
{
    return Qt::DisplayRole;
}

int ModuleBrowser::hubRole()
{
    return Qt::UserRole;
}

int ModuleBrowser::pathRole()
{
    return Qt::UserRole + 1;
}

int ModuleBrowser::categoryRole()
{
    return Qt::UserRole + 2;
}

int ModuleBrowser::descriptionRole()
{
    return Qt::UserRole + 3;
}

int ModuleBrowser::toolTipRole()
{
    return Qt::ToolTipRole;
}

ModuleBrowser::ModuleBrowser(QWidget *parent): QWidget(parent), ui(new Ui::ModuleBrowser)
{
    m_primaryHub = vistle::message::Id::Invalid;
    readSettings();

    ui->setupUi(this);
    m_moduleListWidget[Main] = ui->moduleListWidget;
    m_moduleListWidget[Filtered] = ui->filteredModuleListWidget;

    connect(filterEdit(), SIGNAL(textChanged(QString)), SLOT(setFilter(QString)));
    filterEdit()->installEventFilter(this);
    setFocusProxy(filterEdit());
    ui->filterEdit->installEventFilter(this);

    for (auto idx: ModuleLists) {
        m_moduleListWidget[idx]->setFocusProxy(filterEdit());
        m_moduleListWidget[idx]->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_moduleListWidget[idx], &ModuleListWidget::customContextMenuRequested, this,
                &ModuleBrowser::prepareMenu);
        connect(m_moduleListWidget[idx], &ModuleListWidget::showStatusTip, this, &ModuleBrowser::showStatusTip);
    }
}

ModuleBrowser::~ModuleBrowser()
{
    writeSettings();

    delete ui;
}

ModuleBrowser::WidgetIndex ModuleBrowser::visibleWidgetIndex() const
{
    if (m_filter.trimmed().isEmpty())
        return Main;
    return Filtered;
}

void ModuleBrowser::prepareMenu(const QPoint &pos)
{
    auto *widget = m_moduleListWidget[visibleWidgetIndex()];
    QTreeWidgetItem *item = widget->itemAt(pos);
    for (auto hub: m_hubItems[visibleWidgetIndex()]) {
        int id = hub.first;
        if (hub.second == item) {
            QMenu menu(this);
            if (id != vistle::message::Id::MasterHub) {
                auto *rmAct = new QAction(tr("Remove"), this);
                connect(rmAct, &QAction::triggered, [this, id]() { emit requestRemoveHub(id); });
                menu.addAction(rmAct);
            }
            auto *dbgAct = new QAction(tr("Attach Debugger to Manager"), this);
            connect(dbgAct, &QAction::triggered, [this, id]() { emit requestAttachDebugger(id); });
            menu.addAction(dbgAct);

            menu.exec(widget->mapToGlobal(pos));
            break;
        }
    }
}

std::vector<std::pair<int, QString>> ModuleBrowser::getHubs() const
{
    std::vector<std::pair<int, QString>> hubs;

    for (const auto &h: m_hubItems[Main]) {
        hubs.emplace_back(h.first, h.second->text(0));
    }
    return hubs;
}

QTreeWidgetItem *ModuleBrowser::getHubItem(int hub, ModuleBrowser::WidgetIndex idx) const
{
    auto it = m_hubItems[idx].find(hub);
    if (it == m_hubItems[idx].end())
        return nullptr;
    return it->second;
}

QTreeWidgetItem *ModuleBrowser::getCategoryItem(int hub, QString category, ModuleBrowser::WidgetIndex idx) const
{
    auto hubItem = getHubItem(hub, idx);
    if (!hubItem)
        return nullptr;
    for (int i = 0; i < hubItem->childCount(); ++i) {
        auto name = hubItem->child(i)->data(0, ModuleBrowser::nameRole()).toString();
        if (name == category)
            return hubItem->child(i);
    }
    return nullptr;
}

void ModuleBrowser::addHub(int hub, QString hubName, int nranks, QString address, int port, QString logname,
                           QString realname, bool hasUi, QString systype, QString arch, QString info)
{
    for (auto idx: ModuleLists) {
        auto it = m_hubItems[idx].find(hub);
        if (it == m_hubItems[idx].end()) {
            it = m_hubItems[idx].emplace(hub, new QTreeWidgetItem({hubName}, Hub)).first;
            m_moduleListWidget[idx]->addTopLevelItem(it->second);
        }

        auto &item = it->second;
        if (hasUi) {
            auto label = QString("%1 (%2)").arg(realname).arg(hubName);
            item->setData(0, Qt::DisplayRole, label);
        } else {
            item->setData(0, Qt::DisplayRole, hubName);
        }
        item->setExpanded(m_hubItems[idx].size() <= 1);
        item->setBackground(0, Module::hubColor(hub));
        item->setForeground(0, QColor(0, 0, 0));

        QString sysicon(systype);
        if (systype != "windows" && systype != "linux" && systype != "freebsd" && systype != "apple")
            sysicon = "question";

        QString tt;
        tt += "<b>" + QString::fromStdString(vistle::message::Id::toString(hub)).toHtmlEscaped() + "</b> " +
              QString("(" + QString::number(hub) + ")").toHtmlEscaped();
        tt += QString("<table><tr><td><img height=\"48\" src=\":/icons/%0.svg\"/>&nbsp;</td><td><br>%1<br>%2 "
                      "ranks</td></table><br>")
                  .arg(sysicon)
                  .arg(arch)
                  .arg(nranks);
        tt += "<small>" + info + "</small><br>";
        tt += QString(realname).toHtmlEscaped() + QString(" (%0)").arg(logname) + "<br>";
        tt += QString(hubName).toHtmlEscaped() + QString(":%0").arg(port);
        item->setData(0, Qt::ToolTipRole, tt);

        QPixmap icon;
        if (nranks == 0) {
            icon = QPixmap(":/icons/proxy.svg");
        } else if (nranks == 1) {
            if (hasUi) {
                icon = QPixmap(":/icons/display.svg");
            } else {
                icon = QPixmap(":/icons/server.svg");
            }
        } else {
            if (hasUi)
                icon = QPixmap(":/icons/cluster_ui.svg");
            else
                icon = QPixmap(":/icons/cluster.svg");
        }
        item->setIcon(0, icon);

        if (idx == Main && hub == vistle::message::Id::MasterHub) {
            m_moduleListWidget[idx]->setCurrentItem(item);
            item->setSelected(true);
        }
    }
}

void ModuleBrowser::removeHub(int hub)
{
    for (auto idx: ModuleLists) {
        auto it = m_hubItems[idx].find(hub);
        if (it == m_hubItems[idx].end())
            continue;

        auto &item = it->second;
        delete item;
        m_hubItems[idx].erase(it);
    }
}

QTreeWidgetItem *ModuleBrowser::addCategory(int hub, QString category, QString description,
                                            ModuleBrowser::WidgetIndex idx)
{
    auto *hubItem = getHubItem(hub, idx);
    if (!hubItem) {
        assert(vistle::message::Id::isHub(hub));
        auto it = m_hubItems[idx].emplace(hub, new QTreeWidgetItem({QString::number(hub)}, Hub)).first;
        m_moduleListWidget[idx]->addTopLevelItem(it->second);
        hubItem = it->second;
    }
    assert(hubItem);

    auto item = new CategoryItem(hubItem, {category}, Category);
    item->setData(0, hubRole(), hub);
    item->setData(0, categoryRole(), category);
    item->setData(0, descriptionRole(), description);
    QString tt;
    if (!description.isEmpty()) {
        tt += description.toHtmlEscaped();
    }
    item->setData(0, Qt::ToolTipRole, tt);
    if (idx == Main && hub == m_primaryHub) {
        item->setExpanded(m_categoryExpanded[category.toStdString()]);
    }

    setFilter(m_filter);

    return item;
}

void ModuleBrowser::addModule(int hub, QString module, QString path, QString category, QString description)
{
    if (m_primaryHub == vistle::message::Id::Invalid || m_primaryHub < hub) {
        m_primaryHub = hub;
    }

    for (auto idx: ModuleLists) {
        auto catItem = getCategoryItem(hub, category, idx);
        if (!catItem) {
            std::string cat = category.toStdString();
            std::string desc;
            auto it = CategoryDescriptions.find(cat);
            if (it != CategoryDescriptions.end())
                desc = it->second;
            catItem = addCategory(hub, category, QString::fromStdString(desc), idx);
        }

        for (int i = 0; i < catItem->childCount(); i++) {
            if (catItem->child(i)->data(0, ModuleBrowser::nameRole()).toString() == module)
                continue;
        }

        auto item = new QTreeWidgetItem(catItem, {module}, Module);
        item->setData(0, hubRole(), hub);
        item->setData(0, categoryRole(), category);
        item->setData(0, descriptionRole(), description);
        QString tt;
        if (!description.isEmpty() || !category.isEmpty()) {
            tt += "<p>";
            if (!description.isEmpty())
                tt += description.toHtmlEscaped();
            if (!category.isEmpty())
                tt += "<br><i><b>" + category.toHtmlEscaped() + "</b></i>";
            tt += "</p>";
        }
        tt += "<small><i>" + path.toHtmlEscaped() + "</i></small>";

        item->setData(0, Qt::ToolTipRole, tt);
        m_moduleListWidget[idx]->sortItems(0, Qt::AscendingOrder);
    }
    setFilter(m_filter);
}

QLineEdit *ModuleBrowser::filterEdit() const
{
    return ui->filterEdit;
}

void ModuleBrowser::setFilter(QString filter)
{
    m_filter = filter;

    ui->filteredModuleListWidget->setFilter(m_filter);
    ui->stackedWidget->setCurrentIndex(visibleWidgetIndex());
}

bool ModuleBrowser::eventFilter(QObject *object, QEvent *event)
{
    if (object == filterEdit()) {
        if (event->type() == QEvent::FocusIn) {
            m_filterInFocus = true;
        }
        if (event->type() == QEvent::FocusOut) {
            m_filterInFocus = false;
        }
        if (auto keyEvent = dynamic_cast<QKeyEvent *>(event)) {
            return handleKeyPress(keyEvent);
        }
    }
    return false;
}

bool ModuleBrowser::handleKeyPress(QKeyEvent *event)
{
    if (!m_filterInFocus)
        return false;

    bool press = false;
    if (event->type() == QEvent::KeyPress) {
        press = true;
    }

    auto mod = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        if (mod == Qt::KeyboardModifier::NoModifier) {
            switch (event->key()) {
            case Qt::Key_Escape: {
                // clear filter text
                filterEdit()->setText("");
                return true;
                break;
            }
            case Qt::Key_Left:
            case Qt::Key_Right:
                if (!m_filter.isEmpty()) {
                    // navigate in filter text
                    break;
                }
                // fall through
            case Qt::Key_Down:
            case Qt::Key_Up: {
                // navigate in module tree
                if (press)
                    m_moduleListWidget[visibleWidgetIndex()]->keyPressEvent(event);
                else
                    m_moduleListWidget[visibleWidgetIndex()]->keyReleaseEvent(event);
                return true;
                break;
            }
            case Qt::Key_Return:
            case Qt::Key_Enter: {
                auto selection = m_moduleListWidget[visibleWidgetIndex()]->selectedItems();
                if (!selection.isEmpty()) {
                    assert(selection.size() == 1);
                    auto item = selection.front();
                    if (item->type() == Module) {
                        if (press) {
                            emit startModule(item->data(0, hubRole()).toInt(), item->text(0), Qt::Key_Down);
                        }
                    }
                }
                return true;
                break;
            }
            default:
                break;
            }
        } else if (mod == Qt::KeyboardModifier::ShiftModifier || mod == Qt::KeyboardModifier::AltModifier) {
            switch (event->key()) {
            case Qt::Key_Down:
            case Qt::Key_Up:
            case Qt::Key_Left:
            case Qt::Key_Right: {
                auto selection = m_moduleListWidget[visibleWidgetIndex()]->selectedItems();
                if (!selection.isEmpty()) {
                    assert(selection.size() == 1);
                    auto item = selection.front();
                    if (item->type() == Module) {
                        if (press) {
                            emit startModule(item->data(0, hubRole()).toInt(), item->text(0),
                                             static_cast<Qt::Key>(event->key()));
                        }
                    }
                }
                return true;
                break;
            }
            default:
                break;
            }
        }
    }

    return false;
}

void ModuleBrowser::readSettings()
{
    QSettings settings;
    settings.beginGroup("ModuleBrowser");

    QStringList categories = settings.childKeys();
    for (const auto &cat: categories) {
        m_categoryExpanded[cat.toStdString()] = settings.value(cat).toBool();
    }

    settings.endGroup();
}

void ModuleBrowser::writeSettings()
{
    QSettings settings;
    settings.beginGroup("ModuleBrowser");

    if (m_primaryHub != vistle::message::Id::Invalid) {
        auto hubItem = getHubItem(m_primaryHub);
        for (int i = 0; i < hubItem->childCount(); ++i) {
            auto *child = hubItem->child(i);
            auto cat = child->text(0).toStdString();
            m_categoryExpanded[cat] = child->isExpanded();
        }
    }

    for (const auto &cat: m_categoryExpanded) {
        settings.setValue(cat.first.c_str(), cat.second);
    }

    settings.endGroup();
}

} // namespace gui
