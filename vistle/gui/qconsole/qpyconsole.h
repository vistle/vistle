 /*
  QPyConsole.h

  QConsole specialization for Python

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

#ifndef QPYCONSOLE_H
#define QPYCONSOLE_H

#include "qconsole.h"

/**An emulated singleton console for Python within a Qt application (based on the QConsole class)
 *@author Mondrian Nuessle
 */

class VistleConsole : public QConsole
{
public:
    //destructor
    ~VistleConsole();

    //private constructor
    VistleConsole(QWidget *parent = NULL,
               const QString& welcomeText = "");

    //get the QPyConsole instance
    static VistleConsole *the();

    void printHistory();

protected:
    //give suggestions to complete a command (not working...)
    QStringList suggestCommand(const QString &cmd, QString& prefix);

    //execute a validated command
    QString interpretCommand(const QString &command, int *res);

    void setNormalPrompt(bool display) { setPrompt(">>", display); }
    void setMultilinePrompt(bool display) { setPrompt("...", display); }

private:

    //The instance
    static VistleConsole *s_instance;

private:
    // function to check if current command compiled and if not hinted for a multiline statement
    bool py_check_for_unexpected_eof();

    // string holding the current command
    QString command;

    // number of lines associated with current command
    int lines;
};

#endif
