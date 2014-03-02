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


const char *ModuleBrowser::mimeFormat() {
   return "application/x-modulebrowser";
}

int ModuleBrowser::nameRole() {
   return 1;
}

ModuleBrowser::ModuleBrowser(QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::ModuleBrowser)
{
   ui->setupUi(this);
}

ModuleBrowser::~ModuleBrowser()
{
   delete ui;
}

void ModuleBrowser::addModule(QString module) {

    ui->moduleListWidget->addItem(module);
}

} // namespace gui
