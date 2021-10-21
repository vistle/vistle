/* -*- mode: c++ -*- */
/***************************************************************************
													qconsole.h  -  description
														 -------------------
		begin                : mar mar 15 2005
		copyright            : (C) 2005 by Houssem BDIOUI
		email                : houssem.bdioui@gmail.com
 ***************************************************************************/

// migrated to Qt4 by YoungTaek Oh. date: Nov 29, 2010

/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
 */

#ifndef QCONSOLE_H
#define QCONSOLE_H

#include <QStringList>
#include <QTextEdit>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QEventLoop>

#include <QDialog>
#include <QListWidget>
#include <QDebug>

#if QT_VERSION < 0x040000
#error "supports only Qt 4.0 or greater"
#endif

/**
 * Subclasssing QListWidget
 *
 * @author YoungTaek Oh
 */
class PopupListWidget: public QListWidget {
    Q_OBJECT

public:
    PopupListWidget(QWidget *parent = 0): QListWidget(parent)
    {
        setUniformItemSizes(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    virtual ~PopupListWidget() {}

protected:
    virtual QSize sizeHint() const;
    virtual void keyPressEvent(QKeyEvent *e)
    {
        if (e->key() == Qt::Key_Tab || e->key() == Qt::Key_Return)
            Q_EMIT itemActivated(currentItem());
        else
            QListWidget::keyPressEvent(e);
    }
};

/**
 * An abstract Qt console
 * @author Houssem BDIOUI
 */
class QConsole: public QTextEdit {
    Q_OBJECT
public:
    //constructor
    QConsole(QWidget *parent = NULL, const QString &welcomeText = "");
    //set the prompt of the console
    void setPrompt(const QString &prompt, bool display = true);
    //execCommand(QString) executes the command and displays back its result
    bool execCommand(const QString &command, bool writeCommand = true, bool showPrompt = true, QString *result = NULL);
    //saves a file script
    int saveScript(const QString &fileName);
    //loads a file script
    int loadScript(const QString &fileName);
    //clear & reset the console (useful sometimes)
    void clear();
    void reset(const QString &welcomeText = "");

    //cosmetic methods !

    // @{
    /// get/set command color
    QColor cmdColor() const { return cmdColor_; }
    void setCmdColor(QColor c) { cmdColor_ = c; }
    // @}

    // @{
    /// get/set error color
    QColor errColor() const { return errColor_; }
    void setErrColor(QColor c) { errColor_ = c; }
    // @}

    // @{
    /// get/set info color
    QColor infoColor() const { return infoColor_; }
    void setInfoColor(QColor c) { infoColor_ = c; }
    // @}

    // @{
    /// get/set output color
    QColor outColor() const { return outColor_; }
    void setOutColor(QColor c) { outColor_ = c; }
    // @}
    void setCompletionColor(QColor c) { completionColor = c; }

    // @{
    /// get set font
    void setFont(const QFont &f);
    QFont font() const { return currentFont(); }
    // @}

    void correctPathName(QString &pathName);

    QString getRawInput();

private:
    void dropEvent(QDropEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);

    void keyPressEvent(QKeyEvent *e);
    void contextMenuEvent(QContextMenuEvent *event);

    //Return false if the command is incomplete (e.g. unmatched braces)
    virtual bool isCommandComplete(const QString &command);

    //Test whether the cursor is in the edition zone
    bool isInEditionZone();
    bool isInEditionZone(const int &pos);

    //Test whether the selection is in the edition zone
    bool isSelectionInEditionZone();
    //Change paste behaviour
    void insertFromMimeData(const QMimeData *);


    //protected attributes
protected:
    //colors
    QColor cmdColor_, errColor_, outColor_, completionColor, infoColor_;

    int oldPosition;
    // cached prompt length
    int promptLength;
    // The prompt string
    QString prompt;
    // The commands history
    QStringList history;
    //Contains the commands that has succeeded
    QStringList recordedScript;
    // Current history index (needed because afaik QStringList does not have such an index)
    int historyIndex;
    //Holds the paragraph number of the prompt (useful for multi-line command handling)
    int promptParagraph;

protected:
    //Get the command to validate
    virtual QString getCurrentCommand();

    //Replace current command with a new one
    virtual void replaceCurrentCommand(const QString &newCommand);

    //Implement paste with middle mouse button
    void mousePressEvent(QMouseEvent *);

    //execute a validated command (should be reimplemented and called at the end)
    //the return value of the function is the string result
    //res must hold back the return value of the command (0: passed; else: error)
    virtual QString interpretCommand(const QString &command, int *res);

    //give suggestions to autocomplete a command (should be reimplemented)
    //the return value of the function is the string list of all suggestions
    //the returned prefix is useful to complete "sub-commands"
    virtual QStringList suggestCommand(const QString &cmd, QString &prefix);


public Q_SLOTS:
    //Contextual menu slots
    void cut();
    //void paste();
    void del();
    //displays the prompt
    void displayPrompt();

Q_SIGNALS:
    //Signal emitted after that a command is executed
    void commandExecuted(const QString &command);

private:
    void handleTabKeyPress();
    void handleReturnKeyPress();
    bool handleBackspaceKeyPress();
    void handleUpKeyPress();
    void handleDownKeyPress();
    void setHome(bool);

    QEventLoop rawEventLoop;
};

#endif
