#include "modulebrowser.h"
#include "ui_modulebrowser.h"

#include <QMimeData>
#include <QDebug>

namespace gui {

ModuleListWidget::ModuleListWidget(QWidget *parent)
   : QListWidget(parent)
{
}

QMimeData *ModuleListWidget::mimeData(const QList<QListWidgetItem *> dragList) const
{
   if (dragList.empty())
      return nullptr;

   QMimeData *md = QListWidget::mimeData(dragList);

   QByteArray encodedData;
   QDataStream stream(&encodedData, QIODevice::WriteOnly);
   for(QListWidgetItem *item: dragList)
   {
      stream << item->text();
   }
   md->setData(ModuleBrowser::mimeFormat(), encodedData);
   return md;
}

void ModuleListWidget::setFilter(QString filter) {

   m_filter = filter.trimmed();

   for (int idx=0; idx<count(); ++idx) {

      const auto i = item(idx);
      filterItem(i);
   }
}


void ModuleListWidget::filterItem(QListWidgetItem *item) const {

   const auto name = item->data(ModuleBrowser::nameRole()).toString();
   const bool match = name.contains(m_filter, Qt::CaseInsensitive);
   item->setHidden(!match);
}


const char *ModuleBrowser::mimeFormat() {
   return "application/x-modulebrowser";
}

int ModuleBrowser::nameRole() {

   return Qt::DisplayRole;
}

ModuleBrowser::ModuleBrowser(QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::ModuleBrowser)
{
   ui->setupUi(this);

   connect(filterEdit(), SIGNAL(textChanged(QString)), SLOT(setFilter(QString)));
   ui->moduleListWidget->setFocusProxy(filterEdit());
}

ModuleBrowser::~ModuleBrowser()
{
   delete ui;
}

void ModuleBrowser::addModule(QString module) {

    QListWidgetItem *item = new QListWidgetItem(module);
    ui->moduleListWidget->addItem(item);
    ui->moduleListWidget->filterItem(item);
}

QLineEdit *ModuleBrowser::filterEdit() const {

   return ui->filterEdit;
}

void ModuleBrowser::setFilter(QString filter) {

   ui->moduleListWidget->setFilter(filter);
}

} // namespace gui
