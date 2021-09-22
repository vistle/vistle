#include "vistledoublepropertymanager.h"

VistleDoublePropertyManager::VistleDoublePropertyManager(QObject *parent): QtDoublePropertyManager(parent)
{}

QString VistleDoublePropertyManager::valueText(const QtProperty *property) const
{
    const double val = value(property);
    const int dec = decimals(property);
    //return QLocale::system().toString(val, 'g', dec);
    return QString::number(val, 'g');
}
