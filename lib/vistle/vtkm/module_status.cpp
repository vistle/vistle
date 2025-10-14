#include "module_status.h"


ModuleStatus::ModuleStatus(const std::string &message): msg(message)
{}

ModuleStatus::~ModuleStatus() = default;

const char *ModuleStatus::message() const
{
    return msg.c_str();
}

SuccessStatus::SuccessStatus(): ModuleStatus("")
{}

bool SuccessStatus::continueExecution() const
{
    return true;
}

const vistle::message::SendText::TextType SuccessStatus::messageType() const
{
    return vistle::message::SendText::Info;
}


ErrorStatus::ErrorStatus(const std::string &message): ModuleStatus(message)
{}

bool ErrorStatus::continueExecution() const
{
    return false;
}

const vistle::message::SendText::TextType ErrorStatus::messageType() const
{
    return vistle::message::SendText::Error;
}


WarningStatus::WarningStatus(const std::string &message): ModuleStatus(message)
{}

bool WarningStatus::continueExecution() const
{
    return true;
}
const vistle::message::SendText::TextType WarningStatus::messageType() const
{
    return vistle::message::SendText::Warning;
}


InfoStatus::InfoStatus(const std::string &message): ModuleStatus(message)
{}

bool InfoStatus::continueExecution() const
{
    return true;
}
const vistle::message::SendText::TextType InfoStatus::messageType() const
{
    return vistle::message::SendText::Info;
}


UnsupportedCellTypeErrorStatus::UnsupportedCellTypeErrorStatus(bool isPolyhedral)
: ErrorStatus("Grid contains unsupported cell type.")
{
    if (isPolyhedral) {
        msg = "Grid contains polyhedral cells, which are not supported by Viskores. "
              "Please use the version of the module without the Vtkm suffix or "
              "apply the SplitPolyhedra module to the input before passing it to this module.";
    }
}

ModuleStatusPtr Success()
{
    return std::make_unique<SuccessStatus>();
}

ModuleStatusPtr Error(const std::string &message)
{
    return std::make_unique<ErrorStatus>(message);
}

ModuleStatusPtr Warning(const std::string &message)
{
    return std::make_unique<WarningStatus>(message);
}

ModuleStatusPtr Info(const std::string &message)
{
    return std::make_unique<InfoStatus>(message);
}

ModuleStatusPtr UnsupportedCellTypeError(bool isPolyhedral)
{
    return std::make_unique<UnsupportedCellTypeErrorStatus>(isPolyhedral);
}
