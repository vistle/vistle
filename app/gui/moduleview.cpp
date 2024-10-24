#include "moduleview.h"
#include "ui_moduleview.h"
#include <QPushButton>
#include <QRegularExpression>
#include <vistle/core/message.h>

namespace gui {

ModuleView::ModuleView(QWidget *parent): QWidget(parent), ui(new Ui::ModuleView)
{
    ui->setupUi(this);

    auto *bb = ui->buttonBox;
    m_deleteButton = bb->addButton("Delete", QDialogButtonBox::DestructiveRole);
    m_executeButton = bb->addButton("Execute", QDialogButtonBox::ApplyRole);
    m_defaultsButton = bb->addButton("Defaults", QDialogButtonBox::ResetRole);
    connect(bb, SIGNAL(clicked(QAbstractButton *)), SLOT(buttonClicked(QAbstractButton *)));

    auto *mbb = ui->messagesButtonBox;
    m_messagesButton = mbb->addButton("Messages", QDialogButtonBox::ActionRole);
    m_messagesButton->setEnabled(false);
    m_clearButton = mbb->addButton("Clear", QDialogButtonBox::ActionRole);
    m_replayOutput = mbb->addButton("Replay", QDialogButtonBox::ActionRole);
    m_streamOutput = mbb->addButton("Stream", QDialogButtonBox::ActionRole);
    m_streamOutput->setCheckable(true);
    m_streamOutput->setChecked(false);
    connect(mbb, SIGNAL(clicked(QAbstractButton *)), SLOT(buttonClicked(QAbstractButton *)));

    ui->messages->setVisible(false);
    ui->splitter->setCollapsible(0, false);
    ui->splitter->setCollapsible(1, true);
    // hide messages when splitter is moved to bottom
    connect(ui->splitter, &QSplitter::splitterMoved, [this](int pos, int index) {
        if (index != 1)
            return;
        if (pos > ui->splitter->height() - 50) {
            ui->messages->setVisible(false);
            if (ui->messages->height() < 100) {
                ui->messages->resize(ui->messages->width(), 100);
                auto sizes = ui->splitter->sizes();
                if (sizes[1] < 50) {
                    sizes[0] -= 100;
                    sizes[1] = 100;
                    ui->splitter->setSizes(sizes);
                }
            }
            updateMessageButton();
        }
    });

    setId(vistle::message::Id::Invalid);

    m_defaultsButton->setVisible(false);
    m_deleteButton->setVisible(false);
}

ModuleView::~ModuleView()
{
    delete ui;
}

QScrollArea *ModuleView::parameterScroller() const
{
    return ui->parameterScroller;
}

void ModuleView::buttonClicked(QAbstractButton *button)
{
    if (button == m_messagesButton) {
        ui->messages->setVisible(!ui->messages->isVisible());
        updateMessageButton();
    }
    if (button == m_deleteButton) {
        emit deleteModule(m_id);
    }
    if (button == m_executeButton) {
        emit executeModule(m_id);
    }
    if (button == m_defaultsButton) {
        emit setDefaultValues(m_id);
    }
    if (button == m_clearButton) {
        emit clearMessages(m_id);
        m_messages.clear();
        ui->messages->clear();
        updateMessageButton();
    }
    if (button == m_streamOutput) {
        emit toggleOutputStreaming(m_id, m_streamOutput->isChecked());
    }
    if (button == m_replayOutput) {
        ui->messages->setVisible(true);
        emit replayOutput(m_id);
    }
}

int ModuleView::id() const
{
    return m_id;
}

void ModuleView::setId(int id)
{
    m_id = id;
    bool visible = vistle::message::Id::isModule(id);
    ui->buttonBox->setVisible(visible);
    ui->messagesLabel->setVisible(visible);
    ui->messagesButtonBox->setVisible(visible);
    //m_deleteButton->setVisible(visible);
    m_executeButton->setVisible(visible);
    m_messagesButton->setVisible(visible);
    m_streamOutput->setVisible(visible);
    m_replayOutput->setVisible(visible);
    //ui->parameters->setModule(id);
    ui->messages->setVisible(false);
}

void ModuleView::setOutputStreaming(int id, bool enable)
{
    if (m_id == id) {
        m_streamOutput->setChecked(enable);
    }
}

void ModuleView::setColor(int type)
{
    QColor text = QPalette().color(QPalette::WindowText);

    using vistle::message::SendText;
    switch (type) {
    case SendText::Cerr:
        ui->messages->setTextColor(QColor(Qt::red));
        break;
    case SendText::Cout:
        ui->messages->setTextColor(text);
        break;
    case SendText::Error:
        ui->messages->setTextColor(QColor(Qt::red));
        break;
    case SendText::Warning:
        ui->messages->setTextColor(QColor(Qt::blue));
        break;
    case SendText::Info:
    default:
        ui->messages->setTextColor(text);
        break;
    }
}

void ModuleView::updateMessageButton()
{
    m_messagesButton->setEnabled(!m_messages.empty());

    QString arrow = ui->messages->isVisible() ? "▼" : "▲";
    if (m_messages.size() == 1) {
        m_messagesButton->setText(QString("%1 Message %2").arg(m_messages.size()).arg(arrow));
    } else {
        m_messagesButton->setText(QString("%1 Messages %2").arg(m_messages.size()).arg(arrow));
    }

    m_clearButton->setEnabled(!m_messages.empty());
    //m_clearButton->setVisible(ui->messages->isVisible());

    emit messagesVisibilityChanged(m_id, ui->messages->isVisible());
}

void ModuleView::appendMessage(int senderId, int type, QString text)
{
    if (senderId != m_id)
        return;
    std::cerr << "appendMessage: " << text.toStdString() << std::endl;

    m_messages.push_back({type, text});
#if 1

    setColor(type);
    ui->messages->insertHtml(text);
#else
    ui->messages->clear();
    QString allText;
    for (auto &m: m_messages) {
        setColor(m.type);
        //ui->messages->insertHtml(m.text);
        allText.append(m.text);
    }
    ui->messages->insertHtml(allText);
#endif

    ui->messages->setVisible(true);
    updateMessageButton();
}

void ModuleView::setMessages(QList<Module::Message> &messages, bool visible)
{
    m_messages = messages;

    ui->messages->clear();
    QString text;
    int type = 0;
    for (auto &m: m_messages) {
        if (type != m.type) {
            if (!text.isEmpty()) {
                setColor(type);
                ui->messages->insertHtml(text);
            }
            text.clear();
            type = m.type;
        }
        type = m.type;
        text.append(m.text);
    }
    if (!text.isEmpty()) {
        setColor(type);
        ui->messages->insertHtml(text);
    }
    if (m_messages.empty()) {
        ui->messages->setVisible(false);
    } else {
        ui->messages->setVisible(visible);
    }
    updateMessageButton();
}

} // namespace gui
