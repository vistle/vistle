#ifndef MODULEVIEW_H
#define MODULEVIEW_H

#include <QWidget>
#include "module.h"

class QAbstractButton;
class QPushButton;
class QScrollArea;

namespace gui {

namespace Ui {
class ModuleView;

} //namespace Ui


class ModuleView: public QWidget {
    Q_OBJECT

public:
    explicit ModuleView(QWidget *parent = nullptr);
    ~ModuleView();

    QScrollArea *parameterScroller() const;
    void setId(int id);
    int id() const;
    void setMessages(QList<Module::Message> &messages, bool visible);
signals:
    void executeModule(int id);
    void setDefaultValues(int id);
    void deleteModule(int id);
    void clearMessages(int id);
    void toggleOutputStreaming(int id, bool enable);
    void replayOutput(int id);
    void messagesVisibilityChanged(int id, bool visible);

public slots:
    void appendMessage(int senderId, int type, QString text);
    void setOutputStreaming(int id, bool enable);
private slots:
    void buttonClicked(QAbstractButton *button);

private:
    void setColor(int type);
    void updateMessageButton();

    Ui::ModuleView *ui = nullptr;
    QPushButton *m_executeButton = nullptr;
    QPushButton *m_messagesButton = nullptr;
    QPushButton *m_defaultsButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_streamOutput = nullptr;
    QPushButton *m_replayOutput = nullptr;
    int m_id = 0;

    QList<Module::Message> m_messages;
};

} // namespace gui
#endif
