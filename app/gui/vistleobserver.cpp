/*********************************************************************************/
/*! \file handler.cpp
 *
 * Contains three classes:
 * 1. VistleConnection -- simple class, handling commands dispatched to vistle's userInterface.
 * 2. VistleObserver -- observer class that watches for changes in vistle, and sends
 *    signals to the MainWindow.
 */
/**********************************************************************************/

#include <QString>

#include <vistle/userinterface/vistleconnection.h>

#include "vistleobserver.h"
#include <vistle/core/messages.h>

namespace gui {


/*************************************************************************/
// begin class VistleObserver
/*************************************************************************/

/*!
 * \brief VistleObserver::VistleObserver
 * \param parent
 *
 */
VistleObserver::VistleObserver(QObject *parent): QObject(parent)
{
    m_moduleNames[vistle::message::Id::MasterHub] = "Vistle";
    for (int i = 1; i < 5; ++i) {
        m_moduleNames[vistle::message::Id::MasterHub - i] = "Vistle" + QString::number(i);
    }
}

void VistleObserver::newHub(int hub, const std::string &name, int nranks, const std::string &address,
                            const std::string &logname, const std::string &realname)
{
    QString qname = QString::fromStdString(name);
    QString qaddress = QString::fromStdString(address);
    QString qlogname = QString::fromStdString(logname);
    QString qrealname = QString::fromStdString(realname);
    emit newHub_s(hub, qname, nranks, qaddress, qlogname, qrealname);
}

void VistleObserver::deleteHub(int hub)
{
    emit deleteHub_s(hub);
}

void VistleObserver::moduleAvailable(const vistle::AvailableModule &mod)
{
    emit moduleAvailable_s(mod.hub(), QString::fromStdString(mod.name()), QString::fromStdString(mod.path()),
                           QString::fromStdString(mod.description()));
}

void VistleObserver::newModule(int moduleId, const boost::uuids::uuid &spawnUuid, const std::string &moduleName)
{
    QString name = QString::fromStdString(moduleName);
    m_moduleNames[moduleId] = name;
    emit newModule_s(moduleId, spawnUuid, name);
}

void VistleObserver::deleteModule(int moduleId)
{
    m_moduleNames.erase(moduleId);
    emit deleteModule_s(moduleId);
}

void VistleObserver::moduleStateChanged(int moduleId, int stateBits)
{
    emit moduleStateChanged_s(moduleId, stateBits);
}

void VistleObserver::newParameter(int moduleId, const std::string &parameterName)
{
    QString name = QString::fromStdString(parameterName);
    emit newParameter_s(moduleId, name);
}

void VistleObserver::deleteParameter(int moduleId, const std::string &parameterName)
{
    QString name = QString::fromStdString(parameterName);
    emit deleteParameter_s(moduleId, name);
}

void VistleObserver::parameterValueChanged(int moduleId, const std::string &parameterName)
{
    QString name = QString::fromStdString(parameterName);
    emit parameterValueChanged_s(moduleId, name);
}

void VistleObserver::parameterChoicesChanged(int moduleId, const std::string &parameterName)
{
    QString name = QString::fromStdString(parameterName);
    emit parameterChoicesChanged_s(moduleId, name);
}

void VistleObserver::newPort(int moduleId, const std::string &portName)
{
    QString name = QString::fromStdString(portName);
    emit newPort_s(moduleId, name);
}

void VistleObserver::deletePort(int moduleId, const std::string &portName)
{
    QString name = QString::fromStdString(portName);
    emit deletePort_s(moduleId, name);
}

void VistleObserver::newConnection(int fromId, const std::string &fromName, int toId, const std::string &toName)
{
    QString name = QString::fromStdString(fromName);
    QString name2 = QString::fromStdString(toName);
    emit newConnection_s(fromId, name, toId, name2);
}

void VistleObserver::deleteConnection(int fromId, const std::string &fromName, int toId, const std::string &toName)
{
    QString name = QString::fromStdString(fromName);
    QString name2 = QString::fromStdString(toName);
    emit deleteConnection_s(fromId, name, toId, name2);
}

void VistleObserver::incModificationCount()
{
    bool modStart = modificationCount() == 0;
    StateObserver::incModificationCount();
    if (modStart)
        emit modified(true);
}

void VistleObserver::resetModificationCount()
{
    bool mod = modificationCount() > 0;
    StateObserver::resetModificationCount();
    if (mod)
        emit modified(false);
}

void VistleObserver::loadedWorkflowChanged(const std::string &filename)
{
    emit loadedWorkflowChanged_s(QString::fromStdString(filename));
}

void VistleObserver::sessionUrlChanged(const std::string &url)
{
    emit sessionUrlChanged_s(QString::fromStdString(url));
}

void gui::VistleObserver::info(const std::string &text, vistle::message::SendText::TextType textType, int senderId,
                               int senderRank, vistle::message::Type refType, const vistle::message::uuid_t &refUuid)
{
    QString t = QString::fromStdString(text);
    while (t.endsWith('\n'))
        t.chop(1);
    QString sender;
    if (senderId >= vistle::message::Id::ModuleBase) {
        sender =
            QString("%1_%2(%3)").arg(m_moduleNames[senderId], QString::number(senderId), QString::number(senderRank));
    } else {
        sender = m_moduleNames[senderId];
        if (sender.isEmpty())
            sender = QString("ID %1").arg(QString::number(senderId));
    }
    QString msg = QString("%1: %2").arg(sender, t);
    emit info_s(msg, textType);
}

void VistleObserver::updateStatus(int id, const std::string &text, vistle::message::UpdateStatus::Importance priority)
{
    emit status_s(id, QString::fromStdString(text), priority);
}

void VistleObserver::status(int id, const std::string &text, vistle::message::UpdateStatus::Importance priority)
{
    emit moduleStatus_s(id, QString::fromStdString(text), priority);
}

void VistleObserver::quitRequested()
{
    emit quit_s();
}

void VistleObserver::message(const vistle::message::Message &msg, vistle::buffer *payload)
{
    (void)payload;
    using namespace vistle::message;

    switch (msg.type()) {
    case SCREENSHOT: {
        auto &ss = msg.as<Screenshot>();
        QString file = ss.filename();
        emit screenshot_s(file);
        break;
    }
    default: {
        break;
    }
    }
}

} //namespace gui
