 /*
  vistleconsole.h

  QConsole specialization for Vistle,
  based on QPyConsole

  (C) 2006, Mondrian Nuessle, Computer Architecture Group, University of Mannheim, Germany
  
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
 
   nuessle@uni-mannheim.de

 */

#ifndef VISTLECONSOLE_H
#define VISTLECONSOLE_H

#include "qconsole/qconsole.h"

namespace vistle {
class PythonModule;
}

namespace gui {

/**An emulated singleton console for Python within a Qt application (based on the QConsole class)
 *@author Mondrian Nuessle
 */

class VistleConsole : public QConsole
{
   Q_OBJECT

public:
    //! destructor
    ~VistleConsole();

    //! private constructor
    VistleConsole(QWidget *parent = NULL);

    //! get the QPyConsole instance
    static VistleConsole *the();

    void printHistory();

public slots:
    void appendInfo(const QString &text, int type=-1);
    void appendDebug(const QString &text);

    void setNormalPrompt(bool display) { setPrompt("> ", display); }
protected:
    //! give suggestions to complete a command (not working...)
    QStringList suggestCommand(const QString &cmd, QString& prefix);

    //! execute a validated command
    QString interpretCommand(const QString &command, int *res);

    void setMultilinePrompt(bool display) { setPrompt("... ", display); }

private:

    //! the instance
    static VistleConsole *s_instance;

private:
    //! function to check if current command compiled and if not hinted for a multiline statement
    bool py_check_for_unexpected_eof();

    //! string holding the current command
    QString command;

    //! number of lines associated with current command
    int lines;
};

} // namespace gui
#endif
