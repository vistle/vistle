#include "module_status.h"


ModuleStatus::ModuleStatus(const char *_message): msg(_message)
{}


const char *ModuleStatus::message() const
{
    return msg;
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


ErrorStatus::ErrorStatus(const char *_message): ModuleStatus(_message)
{}

bool ErrorStatus::continueExecution() const
{
    return false;
}
const vistle::message::SendText::TextType ErrorStatus::messageType() const
{
    return vistle::message::SendText::Error;
}


WarningStatus::WarningStatus(const char *_message): ModuleStatus(_message)
{}

bool WarningStatus::continueExecution() const
{
    return true;
}
const vistle::message::SendText::TextType WarningStatus::messageType() const
{
    return vistle::message::SendText::Warning;
}


InfoStatus::InfoStatus(const char *_message): ModuleStatus(_message)
{}

bool InfoStatus::continueExecution() const
{
    return true;
}
const vistle::message::SendText::TextType InfoStatus::messageType() const
{
    return vistle::message::SendText::Info;
}


ModuleStatusPtr Success()
{
    return std::make_unique<SuccessStatus>();
}

ModuleStatusPtr Error(const char *message)
{
    return std::make_unique<ErrorStatus>(message);
}

ModuleStatusPtr Warning(const char *message)
{
    return std::make_unique<WarningStatus>(message);
}

ModuleStatusPtr Info(const char *message)
{
    return std::make_unique<InfoStatus>(message);
}
