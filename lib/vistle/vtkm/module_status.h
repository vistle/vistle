#ifndef MODULE_STATUS_H
#define MODULE_STATUS_H

#include <string>
#include <memory>

#include <vistle/core/messages.h>

//TODO: should  best be moved to lib/vistle/module

/**
 * @class ModuleStatus
 * @brief Base class for representing the status of a Vistle module.
 *
 * This class provides a common interface for representing the success or failure
 * of a module execution in classes which do not inherit from the Module class.
 * ModuleStatus objects can be passed to Vistle modules which then handle the status
 * further, e.g., by sending messages to the GUI or stopping their execution.
 */
class ModuleStatus {
protected:
    const char *msg;

public:
    ModuleStatus(const char *_message);
    virtual ~ModuleStatus() = default;

    // @return True if the module can continue its execution, false otherwise.
    virtual bool continueExecution() const = 0;

    // @return the message associated with the status
    const char *message() const;

    // @return The type of message to be sent to the GUI (e.g., Info, Warning, Error).
    virtual const vistle::message::SendText::TextType messageType() const = 0;
};

typedef std::unique_ptr<ModuleStatus> ModuleStatusPtr;

// Represents a successful execution of a module which does not print any message.
class SuccessStatus: public ModuleStatus {
public:
    SuccessStatus();

    bool continueExecution() const override;
    const vistle::message::SendText::TextType messageType() const override;
};

// Represents a failed execution of a module sending an error message.
class ErrorStatus: public ModuleStatus {
public:
    ErrorStatus(const char *_message);

    bool continueExecution() const override;
    const vistle::message::SendText::TextType messageType() const override;
};

// Represents a successful execution of a module, but prints a warning message.
class WarningStatus: public ModuleStatus {
public:
    WarningStatus(const char *_message);

    bool continueExecution() const override;
    const vistle::message::SendText::TextType messageType() const override;
};

// Represents a successful execution of a module, but prints an informational message.
class InfoStatus: public ModuleStatus {
public:
    InfoStatus(const char *_message);

    bool continueExecution() const override;
    const vistle::message::SendText::TextType messageType() const override;
};

/*
    @return A unique pointer to aI ModuleStatus object representing a successful execution.
*/
ModuleStatusPtr Success();

/*
    @param message A C-string containing the error message.
    @return A unique pointer to a ModuleStatus object representing a failed execution.

*/
ModuleStatusPtr Error(const char *message);

/*
    @param message A C-string containing the warning message.
    @return A unique pointer to a ModuleStatus object representing a module that can continue its execution,
    but prints a warning message.
*/
ModuleStatusPtr Warning(const char *message);

/*
    @param message A C-string containing the informational message.
    @return A unique pointer to a ModuleStatus object representing a successful module that prints some 
    informational message.
*/
ModuleStatusPtr Info(const char *message);

#endif // MODULE_STATUS_H
