#include "parameters.h"
#include "parameterconnectionwidgets.h"

#include <QBuffer>
#include <QDrag>
#include <QHBoxLayout>
#include <QMimeData>
#include <QPainter>
#include <QScrollArea>

#include <iostream>

using namespace gui;

QString gui::displayName(QString parameterName)
{
    return parameterName.replace("_", " ").trimmed();
}

QString gui::parameterName(QString displayName)
{
    return displayName.replace(" ", "_").trimmed();
}

void setParamText(QLabel *label, const QString &paramName, bool connected)
{
    QPixmap iconPixmap(":/icons/parameterconnected.svg");

    int iconHeight = label->fontMetrics().height();
    QPixmap scaledPixmap = iconPixmap.scaledToHeight(0.8 * iconHeight, Qt::SmoothTransformation);

    if (connected) {
        QPalette p;
        QColor color = p.color(QPalette::Active, QPalette::Highlight);
        std::cerr << "color: " << color.red() << " " << color.green() << " " << color.blue() << std::endl;
        // Create a new pixmap with the same size as the scaled pixmap
        QPixmap coloredPixmap(scaledPixmap.size());
        coloredPixmap.fill(Qt::transparent); // Fill with transparent color

        // Use QPainter to draw the original pixmap with a color overlay
        QPainter painter(&coloredPixmap);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.drawPixmap(0, 0, scaledPixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(coloredPixmap.rect(), color); // Set the desired color (e.g., red)
        painter.end();
        scaledPixmap = coloredPixmap;
    }

    // Convert the pixmap to base64
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    scaledPixmap.save(&buffer, "PNG");
    QString base64Data = QString::fromLatin1(byteArray.toBase64().data());
    // Set the HTML content of the label
    QString htmlContent = QString("<img src='data:image/png;base64,%1'> %2").arg(base64Data).arg(paramName);
    label->setText(htmlContent);
}

ParameterConnectionLabel::ParameterConnectionLabel(int moduleId, const QString &paramName, QWidget *parent)
: QLabel(parent), m_moduleId(moduleId), m_paramName(paramName), m_showName(paramName)
{
    setParamText(this, displayName(m_showName), false);
}

void ParameterConnectionLabel::connectParam(int moduleId, const QString &paramName, bool direct)
{
    if (moduleId == m_moduleId && paramName == m_paramName)
        return;
    m_connectedParameters.push_back({moduleId, paramName, direct});
    // setStyleSheet("color: blue; border: 1px solid black; padding: 2px;");
    setParamText(this, displayName(m_showName), true);
    redrawParameterPopup();
}

void ParameterConnectionLabel::disconnectParam(int moduleId, const QString &paramName)
{
    auto it = std::find_if(
        m_connectedParameters.begin(), m_connectedParameters.end(),
        [moduleId, paramName](const Connection &c) { return c.moduleId == moduleId && c.paramName == paramName; });
    if (it != m_connectedParameters.end())
        m_connectedParameters.erase(it);
    if (m_connectedParameters.empty())
        setParamText(this, displayName(m_showName), false);
}

void ParameterConnectionLabel::clearConnections()
{
    m_connectedParameters.clear();
    setParamText(this, displayName(m_showName), false);
    redrawParameterPopup();
}

void ParameterConnectionLabel::hideText()
{
    m_showName = "";
    setParamText(this, displayName(m_showName), false);
}


void ParameterConnectionLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
    }
    QLabel::mousePressEvent(event);
}

void ParameterConnectionLabel::mouseReleaseEvent(QMouseEvent *event)
{
    m_pressed = false;
    if (event->button() != Qt::LeftButton) {
        return;
    }
    initParameterPopup();
    if (m_parameterPopup->isVisible()) {
        m_parameterPopup->close();
        return;
    }
    m_parameterPopup->show();
    m_parameterPopup->setSearchText(m_paramName);
}

bool ParameterConnectionLabel::event(QEvent *event)
{
    if (event->type() == QEvent::Enter) {
        setStyleSheet("background-color: lightgray;");
        for (const auto &param: m_connectedParameters) {
            highlightModule(param.moduleId);
        }
    } else if (event->type() == QEvent::Leave) {
        setStyleSheet("");
        highlightModule(-1);
    } else if (m_pressed && event->type() == QEvent::MouseMove) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;

        // Set the MIME data
        mimeData->setText(QString("Module ID: %1\nParameter Name: %2\n").arg(m_moduleId).arg(m_paramName));
        QByteArray encodedData;
        QDataStream stream(&encodedData, QIODevice::WriteOnly);
        stream << m_moduleId << parameterName(m_paramName);
        mimeData->setData(Parameters::mimeFormat(), encodedData);
        drag->setMimeData(mimeData);

        // Start the drag event
        drag->exec(Qt::CopyAction | Qt::MoveAction);
        m_pressed = false;
    }
    return QLabel::event(event);
}


void ParameterConnectionLabel::initParameterPopup()
{
    QStringList parameters;
    std::vector<bool> withBtn;
    for (const auto &c: m_connectedParameters) {
        parameters.push_back(QString("%1: %2").arg(c.moduleId).arg(c.paramName));
        withBtn.push_back(c.direct);
    }
    if (!m_parameterPopup) {
        m_parameterPopup = std::make_unique<ParameterPopupWithBtn>(parameters, withBtn);
        connect(m_parameterPopup.get(), &ParameterPopupWithBtn::parameterSelected, this,
                [this](const QString &param) { m_parameterPopup->close(); });
        connect(m_parameterPopup.get(), &ParameterPopupWithBtn::parameterHovered, this,
                [this](int moduleId, const QString &param) { emit highlightModule(moduleId); });
        connect(m_parameterPopup.get(), &ParameterPopupWithBtn::parameterDisconnected, this,
                [this](int moduleId, const QString &param) {
                    emit disconnectParameters(m_moduleId, parameterName(m_paramName), moduleId, param);
                });
        m_parameterPopup->setAttribute(Qt::WA_Hover);

    } else {
        m_parameterPopup->setParameters(parameters, withBtn);
    }
    QPoint globalPos = mapToGlobal(QPoint(0, height()));
    m_parameterPopup->move(globalPos);
}

void ParameterConnectionLabel::redrawParameterPopup()
{
    if (m_parameterPopup && m_parameterPopup->isVisible()) {
        initParameterPopup();
    }
}

ParameterListItemWithX::ParameterListItemWithX(const QString &text, QWidget *parent): QWidget(parent), m_text(text)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    m_label = new QLabel(this);
    m_removeButton = new QPushButton("X", this);

    layout->addWidget(m_label);
    layout->addWidget(m_removeButton);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    connect(m_removeButton, &QPushButton::clicked, this, &ParameterListItemWithX::onDisconnectButtonClicked);
}

void ParameterListItemWithX::onDisconnectButtonClicked()
{
    emit disconnectRequested(m_text);
}

QStringList filterParameters(const QStringList &parameters, const QString &query)
{
    std::vector<std::pair<int, QString>> scores;
    for (const auto &param: parameters) {
        if (param.contains(query, Qt::CaseInsensitive)) {
            int pos = param.indexOf(query, 0, Qt::CaseInsensitive);
            scores.push_back({100 - pos, param});
        } else {
            //count number of contained chars
            int score = 0;
            int pos = 0;
            for (const auto &c: query) {
                if (param.contains(c, Qt::CaseInsensitive)) {
                    // prioritize in order
                    auto p = param.indexOf(c, 0, Qt::CaseInsensitive);
                    if (p < pos) {
                        pos = p;
                        ++score;
                    }
                    ++score;
                }
            }
            scores.push_back({score, param});
        }
    }
    std::sort(scores.begin(), scores.end(), [](const auto &a, const auto &b) {
        if (a.first == b.first) {
            return a.second < b.second;
        }
        return a.first > b.first;
    });
    QStringList filteredParameters;
    for (const auto &score: scores) {
        filteredParameters.push_back(score.second);
    }

    return filteredParameters;
}

ParameterPopup::ParameterPopup(const QStringList &parameters)
: QWidget(nullptr, Qt::Popup), m_parameters(parameters), m_layout(new QVBoxLayout(this))
{
    // Add search field
    m_searchField = new QLineEdit(this);
    m_searchField->setPlaceholderText("Search...");

    // Add a search icon using QLineEdit::addAction
    m_searchField->addAction(QIcon(":/icons/search.svg"), QLineEdit::LeadingPosition);


    m_layout->addWidget(m_searchField);
    connect(m_searchField, &QLineEdit::textChanged, this,
            [this](const QString &query) { populateListWidget(filterParameters(m_parameters, query)); });

    // Add list widget
    m_listWidget = new QListWidget(this);
    m_layout->addWidget(m_listWidget);
    connect(m_listWidget, &QListWidget::itemClicked, this,
            [this](QListWidgetItem *item) { emit parameterSelected(parameterName(item->text())); });

    setLayout(m_layout);
    setMaximumHeight(300); // Set the maximum height for the popup

    populateListWidget(m_parameters);
}

QStringList putSystemParamsAtTheEnd(const QStringList &params)
{
    auto parameters = params;
    std::sort(parameters.begin(), parameters.end(), [](const QString &a, const QString &b) {
        if (a.startsWith('_') && !b.startsWith('_')) {
            return false;
        }
        if (!a.startsWith('_') && b.startsWith('_')) {
            return true;
        }
        return a < b;
    });
    return parameters;
}

void ParameterPopup::setParameters(const QStringList &parameters)
{
    m_parameters = putSystemParamsAtTheEnd(parameters);
    populateListWidget(m_parameters);
}

void ParameterPopup::setSearchText(const QString &text)
{
    m_searchField->setText(text);
    m_searchField->textChanged(text);
    m_searchField->setCursorPosition(0);
    m_searchField->selectAll();
    m_searchField->setFocus();
}

void setListWidgetHeigt(QListWidget *listWidget)
{
    const int maxVisibleItems = 5;
    const int itemHeight = listWidget->sizeHintForRow(0);
    const int itemCount = listWidget->count();
    const int visibleItemCount = qMin(itemCount, maxVisibleItems);
    const int totalHeight = visibleItemCount * itemHeight + 2 * listWidget->frameWidth();
    listWidget->setFixedHeight(totalHeight);
}

void ParameterPopup::populateListWidget(const QStringList &parameters)
{
    m_listWidget->clear();

    for (const auto &param: parameters) {
        QListWidgetItem *item = new QListWidgetItem(displayName(param), m_listWidget);
        m_listWidget->addItem(item);
    }
    setListWidgetHeigt(m_listWidget);
}

ParameterPopupWithBtn::ParameterPopupWithBtn(const QStringList &parameters, std::vector<bool> withBtn)
: ParameterPopup(parameters), m_withBtn(withBtn)
{
    m_label = new QLabel(this);
    m_label->setText("<b>Drag onto a module</b> in the workflow view to connect this parameter with one of the "
                     "module's parameters. Connected parameters share their value.");
    m_label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_label->setWordWrap(true);
    m_label->setStyleSheet("padding: 5px");
    m_layout->addWidget(m_label);
    populateListWidget(m_parameters);
}

void ParameterPopupWithBtn::setParameters(const QStringList &parameters, const std::vector<bool> &withBtn)
{
    m_withBtn = withBtn;
    m_parameters = parameters;
    populateListWidget(m_parameters);
}

std::vector<bool> sortLike(const QStringList &sortedParams, const QStringList &baseParams,
                           const std::vector<bool> &baseWithBtn)
{
    std::vector<bool> withBtn;
    for (const auto &param: sortedParams) {
        auto it = std::find(baseParams.begin(), baseParams.end(), param);
        assert(it != baseParams.end());
        auto idx = std::distance(baseParams.begin(), it);
        withBtn.push_back(baseWithBtn[idx]);
    }
    return withBtn;
}

QString wrapText(const QListWidget *listWidget, const QString &text)
{
    QFontMetrics metrics(listWidget->font());
    QStringList words = text.split(' ');
    QString wrappedText;
    QString line;

    for (const QString &word: words) {
        if (metrics.horizontalAdvance(line + word) > listWidget->width()) {
            wrappedText += line + '\n';
            line = word + ' ';
        } else {
            line += word + ' ';
        }
    }
    wrappedText += line.trimmed();
    return wrappedText;
}

void ParameterPopupWithBtn::populateListWidget(const QStringList &parameters)
{
    if (static_cast<size_t>(m_parameters.size()) != m_withBtn.size())
        return;
    m_listWidget->clear();
    if (parameters.empty()) {
        m_searchField->hide();
        // auto headline = new QListWidgetItem(m_listWidget);
        // QLabel *label = new QLabel(m_listWidget);
        // label->setText("<b>Drag onto a module</b>");
        // m_listWidget->setItemWidget(headline, label);
        // QListWidgetItem *widgetItem = new QListWidgetItem(
        //     wrapText(m_listWidget, " in the workflow view to connect this parameter with one of the "
        //                            "module's parameters. Connected parameters share their value."),
        //     m_listWidget);
        // m_listWidget->addItem(widgetItem);
        m_listWidget->hide();
        m_label->show();
    } else {
        m_listWidget->show();
        m_searchField->show();
        m_label->hide();
    }
    auto withBtn = sortLike(parameters, m_parameters, m_withBtn);
    for (const auto &param: parameters) {
        QListWidgetItem *widgetItem = new QListWidgetItem(displayName(param), m_listWidget);

        if (withBtn[&param - &parameters[0]]) {
            auto item = new ParameterListItemWithX(displayName(param), m_listWidget);
            connect(item, &ParameterListItemWithX::disconnectRequested, this, [this](const QString &text) {
                auto moduleId = text.split(":").first().toInt();
                auto paramName = text.split(":").last().trimmed();
                emit parameterDisconnected(moduleId, parameterName(paramName));
            });
            widgetItem->setSizeHint(widgetItem->sizeHint());
            m_listWidget->setItemWidget(widgetItem, item);
        } else {
            m_listWidget->addItem(widgetItem);
        }
    }
    setListWidgetHeigt(m_listWidget);
}

bool ParameterPopupWithBtn::event(QEvent *event)
{
    static QListWidgetItem *lastHoveredItem = nullptr;
    if (event->type() == QEvent::HoverMove) {
        QHoverEvent *hoverEvent = static_cast<QHoverEvent *>(event);
        QPoint listWidgetPos = m_listWidget->mapFrom(this, hoverEvent->pos());
        QListWidgetItem *item = m_listWidget->itemAt(listWidgetPos);
        if (item && item != lastHoveredItem) {
            lastHoveredItem = item;
            auto moduleId = item->text().split(":").first().toInt();
            auto paramName = item->text().split(":").last().trimmed();
            emit parameterHovered(-1, "");
            emit parameterHovered(moduleId, paramName);
        } else if (!item) {
            lastHoveredItem = nullptr;
            emit parameterHovered(-1, "");
        }
    }
    return QWidget::event(event);
}
