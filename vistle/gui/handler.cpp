/*********************************************************************************/
/*! \file handler.cpp
 *
 * Contains three classes:
 * 1. Handler -- currently empty, is intended to be used to handle any instructions
 *    sent to the UiRunner.
 * 2. UiRunner -- simple class, handling commands dispatched to vistle's userInterface.
 * 3. StatePrinter -- observer class that watches for changes in vistle, and sends
 *    signals to the MainWindow.
 */
/**********************************************************************************/
#include "handler.h"

#include <QString>

namespace gui {

/*************************************************************************/
// begin class VHandler
/*************************************************************************/
Handler::Handler()
{
}

/*************************************************************************/
// begin class UiRunner
/*************************************************************************/

UiRunner::UiRunner(vistle::UserInterface &ui) : m_ui(ui)
{
}

void UiRunner::cancel() {
    boost::unique_lock<boost::mutex> lock(m_mutex);
	m_done = true;
}

void UiRunner::operator()() {
	while(m_ui.dispatch()) {
        boost::unique_lock<boost::mutex> lock(m_mutex);
		if (m_done) {
			break;
		}
		usleep(10000);
	}
}

/*************************************************************************/
// begin class StatePrinter
/*************************************************************************/

/*!
 * \brief StatePrinter::StatePrinter
 * \param parent
 *
 */
StatePrinter::StatePrinter(QObject *parent) : QObject(parent)
{

}

void StatePrinter::newModule(int moduleId, const std::string &moduleName) {
	QString name = QString::fromStdString(moduleName);
	emit newModule_s(moduleId, name);
}

void StatePrinter::deleteModule(int moduleId) {
	emit deleteModule_s(moduleId);
}

void StatePrinter::moduleStateChanged(int moduleId, int stateBits) {
    if (stateBits & StateObserver::Initialized) emit moduleStateChanged_s(moduleId, stateBits, INITIALIZED);
    if (stateBits & StateObserver::Killed) emit moduleStateChanged_s(moduleId, stateBits, KILLED);;
    if (stateBits & StateObserver::Busy) emit moduleStateChanged_s(moduleId, stateBits, BUSY);;
}

void StatePrinter::newParameter(int moduleId, const std::string &parameterName) {
	QString name = QString::fromStdString(parameterName);
	emit newParameter_s(moduleId, name);
}

void StatePrinter::parameterValueChanged(int moduleId, const std::string &parameterName) {
	QString name = QString::fromStdString(parameterName);
	emit parameterValueChanged_s(moduleId, name);
}

void StatePrinter::newPort(int moduleId, const std::string &portName) {
	QString name = QString::fromStdString(portName);
	emit newPort_s(moduleId, name);
}

void StatePrinter::newConnection(int fromId, const std::string &fromName,
                   				 int toId, const std::string &toName) {
    QString name = QString::fromStdString(fromName);
    QString name2 = QString::fromStdString(toName);
    emit newConnection_s(fromId, name, toId, name2);
}

void StatePrinter::deleteConnection(int fromId, const std::string &fromName,
                      				int toId, const std::string &toName) {
    QString name = QString::fromStdString(fromName);
    QString name2 = QString::fromStdString(toName);
    emit deleteConnection_s(fromId, name, toId, name2);
}

} //namespace gui
