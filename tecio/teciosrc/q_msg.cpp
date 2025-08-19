#include "stdafx.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "CodeContract.h"
#include "Q_UNICODE.h"
 #if defined MSWIN
#include "UnicodeStringUtils.h"
 #endif
#include "ALLOC.h"
#include "STRUTIL.h"
#include "stringformat.h"
using std::string; using namespace tecplot;
 #define MAXCHARACTERSPERLINE 60
___372 ___4475(char const* ___2871, char**      ___2698) { size_t ___2163; if (___2871 == NULL) return (___1303); ___2163 = strlen(___2871); *___2698 = ___23(___2163 + 1, char, "new error message string"); if (*___2698 == NULL) return (___1303); strcpy(*___2698, ___2871); if (___2163 > MAXCHARACTERSPERLINE) { char *LineStart = *___2698; char *LastWord  = *___2698; char *WPtr      = *___2698; while (WPtr && (*WPtr != '\0')) { size_t CurLen; WPtr = strchr(LineStart, '\n'); if (WPtr && ((WPtr - LineStart) < MAXCHARACTERSPERLINE)) { WPtr++; while (*WPtr == '\n') WPtr++; LineStart = WPtr; while (*WPtr == ' ') WPtr++; LastWord  = WPtr; continue; } WPtr = strchr(LastWord, ' '); if (WPtr != NULL) { while (*WPtr == ' ') WPtr++; } if (WPtr == NULL) { CurLen = strlen(LineStart); } else { CurLen = WPtr - LineStart; } if (CurLen > MAXCHARACTERSPERLINE) { if (LastWord == LineStart) { if (WPtr && (*WPtr != '\0')) { *(WPtr - 1) = '\n'; LastWord = WPtr; } } else { *(LastWord - 1) = '\n'; } LineStart = LastWord; } else LastWord = WPtr; } } return (___4224); } namespace { void SendMessageToFile( FILE*            F, char const*      S, MessageBoxType_e messageBoxType) { REQUIRE(VALID_REF(F)); REQUIRE(VALID_REF(S)); REQUIRE(VALID_ENUM(messageBoxType, MessageBoxType_e)); REQUIRE(messageBoxType == ___2441 || messageBoxType == ___2445 || messageBoxType == ___2442); char* S2 = NULL; if (___4475(S, &S2)) { switch (messageBoxType) { case ___2441:       fprintf(F, "Error: %s\n",   S2); break; case ___2445:     fprintf(F, "Warning: %s\n", S2); break; case ___2442: fprintf(F, "Info: %s\n",    S2); break; default: ___476(___1303); break; } ___1528(S2, "temp info string"); } } } namespace { void defaultErrMsg(char const* Msg) { REQUIRE(VALID_REF(Msg));
 #if defined MSWIN
::MessageBoxA(NULL, Msg, "Error", MB_OK | MB_ICONERROR);
 #else
SendMessageToFile(stderr, Msg, ___2441);
 #endif
} } namespace { void postMessage( std::string const& ___2430, MessageBoxType_e   messageBoxType) { { if (messageBoxType == ___2441) defaultErrMsg(___2430.c_str()); else SendMessageToFile(stderr, ___2430.c_str(), messageBoxType); } } } namespace { void Message(char const* ___2430, MessageBoxType_e messageBoxType) { REQUIRE(VALID_NON_ZERO_LEN_STR(___2430)); REQUIRE(VALID_ENUM(messageBoxType, MessageBoxType_e)); REQUIRE(messageBoxType == ___2441 || messageBoxType == ___2445 || messageBoxType == ___2442); static ___372 InMessage = ___1303; if (!InMessage) { { SendMessageToFile(stderr, ___2430, messageBoxType); } } } } void Information(___4216 format, ...) { REQUIRE(!format.___2033()); ___372 cleanUp = ___4224; va_list  ___93;
 #if defined(_MSC_VER)
 #pragma warning (push, 0)
 #pragma warning (disable:4840)
 #endif
va_start(___93, format);
 #if defined(_MSC_VER)
 #pragma warning (pop)
 #endif
char* ___2430 = ___4407(format.c_str(), ___93); va_end(___93); if (___2430 == NULL) { cleanUp = ___1303; ___2430 = (char*)format.c_str(); } Message(___2430, ___2442); if (cleanUp) ___1528(___2430, "message"); } void ___4444(___4216 format, ...) { REQUIRE(!format.___2033()); ___372 cleanUp = ___4224; va_list  ___93;
 #if defined(_MSC_VER)
 #pragma warning (push, 0)
 #pragma warning (disable:4840)
 #endif
va_start(___93, format);
 #if defined(_MSC_VER)
 #pragma warning (pop)
 #endif
char* ___2430 = ___4407(format.c_str(), ___93); va_end(___93); if (___2430 == NULL) { cleanUp = ___1303; ___2430 = (char*)format.c_str(); } Message(___2430, ___2445); if (cleanUp) ___1528(___2430, "message"); } namespace { void PostErrorMessage(___4216 format, va_list          ___93) { REQUIRE(!format.___2033()); ___372 cleanUp = ___4224; char* ___2453 = ___4407(format.c_str(), ___93); if (___2453 == NULL) { cleanUp = ___1303; ___2453 = (char*)format.c_str(); } postMessage(___2453, ___2441); if (cleanUp) ___1528(___2453, "messageString"); } } void ___4404(___4216 format, va_list          ___93) { REQUIRE(!format.___2033()); static ___372 InErrMsg = ___1303; if (!InErrMsg) { PostErrorMessage(format, ___93); } } void ___1175(___4216 format, ...) { REQUIRE(!format.___2033()); va_list ___93;
 #if defined(_MSC_VER)
 #pragma warning (push, 0)
 #pragma warning (disable:4840)
 #endif
va_start(___93, format);
 #if defined(_MSC_VER)
 #pragma warning (pop)
 #endif
PostErrorMessage(format, ___93); va_end(___93); }
