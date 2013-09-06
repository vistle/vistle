#include "qlongspinbox.h"

#include <QLineEdit>

QLongSpinBox::QLongSpinBox( QWidget* parent /*= 0*/ )
: QAbstractSpinBox(parent)
, m_value(0)
, m_step(1)
, m_min()
, m_max()
{
   connect(lineEdit(), SIGNAL(textChanged(QString)),
           this, SLOT(setText(QString)));
}

long QLongSpinBox::value() const
{
   return m_value;
}
 
QString QLongSpinBox::text() const
{
   return QString::number(m_value);
}

void QLongSpinBox::setRange(long min, long max)
{
   m_min = min;
   m_max = max;
   if (m_min > m_max)
      std::swap(m_min, m_max);
}

long QLongSpinBox::singleStep() const
{
   return m_step;
}

void QLongSpinBox::setValue(long val)
{
   m_value = val;
   lineEdit()->setText(QString::number(m_value));
   emit valueChanged(m_value);
   emit valueChanged(text());
}

void QLongSpinBox::setText(QString t)
{
   m_value = t.toLong();
   lineEdit()->setText(QString::number(m_value));
   emit valueChanged(m_value);
   emit valueChanged(text());
}

void QLongSpinBox::setSingleStep(long step)
{
   m_step = step;
}

void QLongSpinBox::stepBy(int steps)
{
   m_value += steps*m_step;
   lineEdit()->setText(QString::number(m_value));
   emit valueChanged(m_value);
   emit valueChanged(text());
}
 
QAbstractSpinBox::StepEnabled QLongSpinBox::stepEnabled() const
{
   return ( StepUpEnabled) | (StepDownEnabled);
}

QValidator::State QLongSpinBox::validate(QString &input, int &pos) const
{
   return QValidator::Acceptable;
}

void QLongSpinBox::fixup(QString &str) const
{
   long val = str.toLong();
   str = QString::number(val);
}
