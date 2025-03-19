#ifndef VISTLE_GUI_PARAMETERCONNECTIONWIDGETS_H
#define VISTLE_GUI_PARAMETERCONNECTIONWIDGETS_H

#include <QPushButton>
#include <QString>
#include <QEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <vector>
#include "qtpropertybrowser.h"
namespace gui {

QString displayName(QString parameterName);

QString parameterName(QString displayName);


class ParameterPopup: public QWidget {
    Q_OBJECT
public:
    ParameterPopup(const QStringList &parameters);
    void setParameters(const QStringList &parameters);
    void setSearchText(const QString &text);
signals:
    void parameterSelected(const QString &param);

protected:
    virtual void populateListWidget(const QStringList &parameters);
    QStringList m_parameters;
    QVBoxLayout *m_layout;
    QListWidget *m_listWidget;
    QLineEdit *m_searchField;
};

class ParameterPopupWithBtn: public ParameterPopup {
    Q_OBJECT
public:
    ParameterPopupWithBtn(const QStringList &parameters, std::vector<bool> withBtn);
    void setParameters(const QStringList &parameters, const std::vector<bool> &withBtn);
signals:
    void parameterSelected(const QString &param);
    void parameterHovered(int moduleId, const QString &param);
    void parameterDisconnected(int moduleId, const QString &param);

private:
    void populateListWidget(const QStringList &parameters) override;
    bool event(QEvent *event) override;
    std::vector<bool> m_withBtn;
    QLabel *m_label;
};

class ParameterListItemWithX: public QWidget {
    Q_OBJECT

public:
    ParameterListItemWithX(const QString &text, QWidget *parent = nullptr);

signals:
    void disconnectRequested(const QString &text);

private slots:
    void onDisconnectButtonClicked();

private:
    QLabel *m_label;
    QPushButton *m_removeButton;
    QString m_text;
};

class ParameterConnectionLabel: public QLabel {
    Q_OBJECT

public:
    ParameterConnectionLabel(int moduleId, const QString &paramName, QWidget *parent = nullptr);
    void connectParam(int moduleId, const QString &paramName, bool direct);
    void disconnectParam(int moduleId, const QString &paramName);
    void clearConnections();
    void hideText();
signals:
    void highlightModule(int moduleId); //sends -1 if no module is to be highlighted
    void disconnectParameters(int fromId, const QString &fromName, int toId, const QString &toName);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    bool event(QEvent *event) override;

private:
    int m_moduleId;
    QString m_paramName, m_showName;
    struct Connection {
        int moduleId;
        QString paramName;
        bool direct;
    };
    std::vector<Connection> m_connectedParameters;
    std::unique_ptr<ParameterPopupWithBtn> m_parameterPopup;
    bool m_pressed = false;
    void initParameterPopup();
    void redrawParameterPopup();
};

} // namespace gui

#endif
