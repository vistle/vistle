#ifndef VISTLE_BROWSEREDIT_H
#define VISTLE_BROWSEREDIT_H

#include <QWidget>
#include <QLineEdit>

#include "../remotefilebrowser/remotefiledialog.h"
#include <vistle/core/message.h>

class QHBoxLayout;
class QLineEdit;
class QToolButton;
class RemoteFileDialog;
class AbstractFileInfoGatherer;
class AbstractFileSystemModel;

namespace vistle {
class UserInterface;
}

class VistleBrowserEdit: public QWidget {
    Q_OBJECT
public:
    enum FileMode { File, ExistingFile, Directory, ExistingDirectory };

    VistleBrowserEdit(QWidget *parent = nullptr);
    ~VistleBrowserEdit() override;
    void setUi(vistle::UserInterface *ui);
    QLineEdit *edit() const;

    QString text() const;
    void setModuleId(int id);
    void setText(const QString &text);
    void setFilters(const QString &filters);
    void setReadOnly(bool ro);
    const QValidator *validator() const;
    void setValidator(QValidator *v);
    void setEchoMode(QLineEdit::EchoMode);
    void setTitle(const QString &title);

    void setFileMode(FileMode fileMode);

signals:
    void editingFinished();

private:
    vistle::UserInterface *m_ui = nullptr;
    int m_moduleId = vistle::message::Id::Invalid;
    QHBoxLayout *m_layout = nullptr;
    QToolButton *m_button = nullptr;
    QLineEdit *m_edit = nullptr;
    FileMode m_fileMode = File;
    QStringList m_nameFilters;
    RemoteFileDialog *m_browser = nullptr;
    AbstractFileInfoGatherer *m_fig = nullptr;
    AbstractFileSystemModel *m_model = nullptr;
    QString m_title;

    void applyFileMode();
    void applyNameFilters();
};
#endif
