#ifndef VISTLE_BROWSEREDIT_H
#define VISTLE_BROWSEREDIT_H

#include <QWidget>
#include <QLineEdit>
#include <QFileDialog>

class QFileDialog;
class QHBoxLayout;
class QLineEdit;
class QToolButton;

class VistleBrowserEdit: public QWidget {
    Q_OBJECT
public:
    typedef QFileDialog::FileMode FileMode;

    VistleBrowserEdit(QWidget *parent = nullptr);
    ~VistleBrowserEdit() override;
    QLineEdit *edit() const;

    QString text() const;
    void setText(const QString &text);
    void setReadOnly(bool ro);
    const QValidator *validator() const;
    void setValidator(QValidator *v);
    void setEchoMode(QLineEdit::EchoMode);

    void setFileMode(FileMode fileMode);

signals:
    void editingFinished();

private:
    QHBoxLayout *m_layout = nullptr;
    QToolButton *m_button = nullptr;
    QLineEdit *m_edit = nullptr;
    FileMode m_fileMode = QFileDialog::AnyFile;
    QFileDialog *m_browser = nullptr;

    void applyFileMode();
};
#endif
