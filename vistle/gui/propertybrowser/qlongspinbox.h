#ifndef Q_LONG_SPIN_BOX_H
#define Q_LONG_SPIN_BOX_H

#include <QWidget>
#include <QAbstractSpinBox>
 
class QLongSpinBox: public QAbstractSpinBox
{
   Q_OBJECT
   Q_PROPERTY(long value READ value WRITE setValue NOTIFY valueChanged USER true)
   Q_PROPERTY(long singleStep READ singleStep WRITE setSingleStep)

public:
   QLongSpinBox(QWidget* parent = 0);

   long value() const;
   QString text() const;
   void setRange(long min, long max);
   long singleStep() const;

public slots:
   void setValue(long val);
   void setText(QString text);
   void setSingleStep(long step);
   void stepBy(int steps);

public:
   QAbstractSpinBox::StepEnabled stepEnabled() const;

signals:
   void valueChanged(long);
   void valueChanged(const QString &);

protected:
   virtual QValidator::State validate(QString &input, int &pos) const;
   virtual void fixup(QString &str) const;

private:
   long m_value;
   long m_step;
   long m_min, m_max;
};

#endif
