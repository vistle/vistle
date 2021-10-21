#ifndef Q_LONG_SPIN_BOX_H
#define Q_LONG_SPIN_BOX_H

#include <QWidget>
#include <QAbstractSpinBox>

#include <vistle/core/scalar.h>

class QLongSpinBox: public QAbstractSpinBox {
    Q_OBJECT
    Q_PROPERTY(vistle::Integer value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(vistle::Integer singleStep READ singleStep WRITE setSingleStep)

public:
    QLongSpinBox(QWidget *parent = 0);

    vistle::Integer value() const;
    QString text() const;
    void setRange(vistle::Integer min, vistle::Integer max);
    vistle::Integer singleStep() const;

public slots:
    void setValue(vistle::Integer val);
    void setText(QString text);
    void setSingleStep(vistle::Integer step);
    void stepBy(int steps);

public:
    QAbstractSpinBox::StepEnabled stepEnabled() const;

signals:
    void valueChanged(vistle::Integer);
    void valueChanged(const QString &);

protected:
    virtual QValidator::State validate(QString &input, int &pos) const;
    virtual void fixup(QString &str) const;

private:
    vistle::Integer m_value;
    vistle::Integer m_step;
    vistle::Integer m_min, m_max;
};

#endif
