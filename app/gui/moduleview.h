#ifndef VISTLE_GUI_MODULEVIEW_H
#define VISTLE_GUI_MODULEVIEW_H

#include <QWidget>
#include "module.h"

class QScrollArea;
class QToolBar;

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
    void help(int id);

public slots:
    void appendMessage(int senderId, int type, QString text);
    void setOutputStreaming(int id, bool enable);

private:
    void setColor(int type);
    void updateMessagesAction();

    Ui::ModuleView *ui = nullptr;

    QToolBar *m_toolbar = nullptr;

    int m_id = 0;
    QList<Module::Message> m_messages;
};

} // namespace gui
#endif
