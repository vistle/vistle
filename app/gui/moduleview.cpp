#include "moduleview.h"
#include "ui_moduleview.h"
#include <QRegularExpression>
#include <QToolBar>
#include <QLabel>
#include <vistle/core/message.h>

namespace gui {

ModuleView::ModuleView(QWidget *parent): QWidget(parent), ui(new Ui::ModuleView)
{
    ui->setupUi(this);

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
            updateMessagesAction();
        }
    });


    m_toolbar = new QToolBar(this);
    auto &tb = m_toolbar;
    ui->verticalLayout->addWidget(tb);

    ui->actionExecute->setToolTip("Execute module");
    connect(ui->actionExecute, &QAction::triggered, [this]() { emit executeModule(m_id); });
    tb->addAction(ui->actionExecute);

    tb->addSeparator();

    auto output = new QLabel(tb);
    output->setText("Output:");
    tb->addWidget(output);
    connect(ui->actionMessages, &QAction::triggered, [this]() {
        ui->messages->setVisible(!ui->messages->isVisible());
        updateMessagesAction();
    });
    ui->actionMessages->setEnabled(false);
    tb->addAction(ui->actionMessages);

    connect(ui->actionClear, &QAction::triggered, [this]() {
        emit clearMessages(m_id);
        m_messages.clear();
        ui->messages->clear();
        ui->messages->setVisible(false);
        updateMessagesAction();
    });
    tb->addAction(ui->actionClear);

    connect(ui->actionReplay, &QAction::triggered, [this]() {
        ui->messages->setVisible(true);
        emit replayOutput(m_id);
    });
    tb->addAction(ui->actionReplay);

    connect(ui->actionStream, &QAction::toggled, [this](bool checked) { emit toggleOutputStreaming(m_id, checked); });
    ui->actionStream->setCheckable(true);
    tb->addAction(ui->actionStream);

    tb->addSeparator();

    connect(ui->actionDefaults, &QAction::triggered, [this]() { emit setDefaultValues(m_id); });
    tb->addAction(ui->actionDefaults);

    connect(ui->actionDelete, &QAction::triggered, [this]() { emit deleteModule(m_id); });
    tb->addAction(ui->actionDelete);

    tb->addSeparator();

    connect(ui->actionHelp, &QAction::triggered, [this]() { emit help(m_id); });
    tb->addAction(ui->actionHelp);

    setId(vistle::message::Id::Invalid);
}

ModuleView::~ModuleView()
{
    delete ui;
}

QScrollArea *ModuleView::parameterScroller() const
{
    return ui->parameterScroller;
}

int ModuleView::id() const
{
    return m_id;
}

void ModuleView::setId(int id)
{
    m_id = id;
    bool visible = vistle::message::Id::isModule(id);
    m_toolbar->setVisible(visible);

#if 0
    ui->messagesLabel->setVisible(visible);
    //ui->actionDelete->setVisible(visible);
    //ui->actionDefaults->setVisible(visible);
    ui->actionExecute->setVisible(visible);
    ui->actionMessages->setVisible(visible);
    ui->actionStream->setVisible(visible);
    ui->actionReplay->setVisible(visible);
    //ui->parameters->setModule(id);
    ui->messages->setVisible(false);
#endif
}

void ModuleView::setOutputStreaming(int id, bool enable)
{
    if (m_id == id) {
        ui->actionStream->setChecked(enable);
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

void ModuleView::updateMessagesAction()
{
    ui->actionMessages->setEnabled(!m_messages.empty());

    QString arrow = ui->messages->isVisible() ? "▼" : "▲";
    if (m_messages.size() == 1) {
        ui->actionMessages->setText(QString("%1 Message %2").arg(m_messages.size()).arg(arrow));
    } else {
        ui->actionMessages->setText(QString("%1 Messages %2").arg(m_messages.size()).arg(arrow));
    }

    ui->actionClear->setEnabled(!m_messages.empty());
    //ui->actionClear->setVisible(ui->messages->isVisible());

    emit messagesVisibilityChanged(m_id, ui->messages->isVisible());
}

void ModuleView::appendMessage(int senderId, int type, QString text)
{
    if (senderId != m_id)
        return;
    //std::cerr << "appendMessage: " << text.toStdString() << std::endl;

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
    updateMessagesAction();
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
    updateMessagesAction();
}

} // namespace gui
