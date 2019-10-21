#include "modulebrowser.h"
#include "ui_modulebrowser.h"
#include "module.h"

#include <QMimeData>
#include <QDebug>

namespace gui {

enum ItemTypes {
    Hub,
    Module,
};

ModuleListWidget::ModuleListWidget(QWidget *parent)
   : QTreeWidget(parent)
{
    header()->setVisible(false);
}

QMimeData *ModuleListWidget::mimeData(const QList<QTreeWidgetItem *> dragList) const
{
   if (dragList.empty())
      return nullptr;

   QMimeData *md = QTreeWidget::mimeData(dragList);

   QByteArray encodedData;
   QDataStream stream(&encodedData, QIODevice::WriteOnly);
   for(QTreeWidgetItem *item: dragList) {
       if (item->type() != Module)
           continue;
       stream << item->data(0, ModuleBrowser::hubRole()).toInt();
       stream << item->text(0);
   }
   md->setData(ModuleBrowser::mimeFormat(), encodedData);
   return md;
}

void ModuleListWidget::setFilter(QString filter) {

    m_filter = filter.trimmed();

    for (int idx=0; idx<topLevelItemCount(); ++idx) {

        auto ti = topLevelItem(idx);
        for (int i=0; i<ti->childCount(); ++i) {
            auto item = ti->child(i);
            filterItem(item);
        }
    }
}


void ModuleListWidget::filterItem(QTreeWidgetItem *item) const {

    if (item->type() != Module) {
        item->setHidden(false);
        return;
    }

    const auto name = item->data(0, ModuleBrowser::nameRole()).toString();
    const bool match = name.contains(m_filter, Qt::CaseInsensitive);
    item->setHidden(!match);
}


const char *ModuleBrowser::mimeFormat() {
   return "application/x-modulebrowser";
}

int ModuleBrowser::nameRole() {

   return Qt::DisplayRole;
}

int ModuleBrowser::hubRole() {

   return Qt::UserRole;
}

int ModuleBrowser::pathRole() {

   return Qt::UserRole+1;
}

ModuleBrowser::ModuleBrowser(QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::ModuleBrowser)
{
   ui->setupUi(this);

   connect(filterEdit(), SIGNAL(textChanged(QString)), SLOT(setFilter(QString)));
   ui->moduleListWidget->setFocusProxy(filterEdit());
   setFocusProxy(filterEdit());
}

ModuleBrowser::~ModuleBrowser()
{
   delete ui;
}

void ModuleBrowser::addModule(int hub, QString hubName, QString module, QString path) {

    auto it = hubItems.find(hub);
    if (it == hubItems.end()) {
        it = hubItems.emplace(hub, new QTreeWidgetItem({hubName}, Hub)).first;
        ui->moduleListWidget->addTopLevelItem(it->second);
        it->second->setExpanded(hubItems.size() <= 1);
        it->second->setBackgroundColor(0, Module::hubColor(hub));
        it->second->setForeground(0, QColor(0,0,0));
        QString tt = hubName;
        tt += " (" + QString::number(hub) + ")";
        it->second->setToolTip(0, tt);
    }
    auto &hubItem = it->second;

    auto item = new QTreeWidgetItem(hubItem, {module}, Module);
    item->setData(0, hubRole(), hub);
    QString tt = path;
    tt += " - " + hubName + " (" + QString::number(hub) + ")";
    item->setData(0, Qt::ToolTipRole, tt);
    ui->moduleListWidget->filterItem(item);
}

QLineEdit *ModuleBrowser::filterEdit() const {

   return ui->filterEdit;
}

void ModuleBrowser::setFilter(QString filter) {

   ui->moduleListWidget->setFilter(filter);
}

} // namespace gui
