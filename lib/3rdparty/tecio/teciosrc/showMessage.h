 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <cstdarg>
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
extern bool _showMessage(MessageBoxType_e messageBoxType, char const* ___2430); namespace tecplot { namespace ___3931 { inline bool messageBox(MessageBoxType_e messageBoxType, char const* format, va_list args) { REQUIRE(VALID_ENUM(messageBoxType, MessageBoxType_e)); REQUIRE(VALID_REF(format)); char ___2453[5000]; vsprintf(___2453, format, args); return _showMessage(messageBoxType, ___2453); } inline bool ___1184(char const* format, ...) { va_list args; va_start(args, format); (void)messageBox(___2441, format, args); va_end(args); return false; } inline bool ___3245(char const* format, ...) { va_list args; va_start(args, format); bool ___2037 = (messageBox(___2447, format, args) == ___4224); va_end(args); return ___2037; } inline bool ___1929(FILE* file, char const* format, ...) { va_list args; va_start(args, format); bool ___2037; if (file == NULL) ___2037 = messageBox(___2442, format, args); else ___2037 = (vfprintf(file, format, args) >= 0); va_end(args); return ___2037; } inline bool ___1929(char const* format, ...) { va_list args; va_start(args, format); bool ___2037 = messageBox(___2442, format, args); va_end(args); return ___2037; } }}
