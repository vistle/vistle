 #pragma once
 #if defined EXTERN
 #undef EXTERN
 #endif
 #if defined ___3860
 #define EXTERN
 #else
 #define EXTERN extern
 #endif
#include "ThirdPartyHeadersBegin.h"
#include <string>
#include <set>
#include <vector>
#include "ThirdPartyHeadersEnd.h"
namespace tecplot { class ___3439; } EXTERN void ___1474(); EXTERN char *___4407(const char *Format, va_list     ___93); EXTERN char *___1473(tecplot::___4216 Format, ...); EXTERN int ___1473(std::string&                       ___417, tecplot::___4216 Format ...); EXTERN char *___1133(tecplot::___4216 ___3811); EXTERN void ___676(char       *___3944, const char *___3640, int         ___1924, int         ___682);
 #if !defined MSWIN
EXTERN void ___3350(char *S, short ___2869, short ___2694);
 #endif
EXTERN void ___2333(char *str); EXTERN void ___2334(char *str); EXTERN char *___4223(char *___3811); EXTERN char *___3816(char *___3811); EXTERN char *___3857(char *___3811, int   ___2374); std::string& ___3856( std::string& str, size_t       maxLength); char* ___3856( char* ___3811, int   ___2374);
 #ifndef MSWIN
EXTERN ___3837 ___2243(const char *___3811, uint32_t    ___4474);
 #endif
EXTERN void SkipSeparator(char const** ___683); EXTERN void ___3607(char const** ___683); EXTERN void ___3606(char const** ___683); EXTERN const char *ustrstr(const char *___3428, const char *___3429); EXTERN int  ustrncmp(const char *___3428, const char *___3429, size_t      ___2221); EXTERN int  ustrcmp(const char *___3428, const char *___3429); ___372 isStringDelimiter(char C);
 #if !defined NO_ASSERTS
EXTERN ___372 ___1980(char       **___3431, const char  *___2698, ___372    ___2061, const char  *___1393, int          ___2260);
 #  define ___3353(___3431, ___2698, ___2061) ___1980( \
 ___3431, \
 ___2698, \
 ___2061, \
 __FILE__, __LINE__)
 #else
EXTERN ___372 ___1980(char       **___3431, const char  *___2698, ___372    ___2061);
 #  define ___3353(___3431, ___2698, ___2061) ___1980( \
 ___3431, \
 ___2698, \
 ___2061)
 #endif
EXTERN ___372 ___3939( char**      ___3431, char const* ___3854, ___372   ___956, ___372   convertNewlineToEscSeq); EXTERN ___372 ___3938( char**      ___3431, char const* ___3854, ___372   convertNewlineToEscSeq); EXTERN ___372 ___3937(char     **___3431, char       ___474); EXTERN ___372 ___3351(char **___3811); EXTERN ___372 ___3349(char **S); EXTERN ___372 ___1185(char **S, char ___957); EXTERN ___372 ___3437(tecplot::___3439  &___3438, char                        ___3913, ___372                   ___2873); EXTERN char *___654(const char *___2882); EXTERN char *___652(const char *___2697, ___372   ___3472); EXTERN char *___1952(char *___339, char *___2685); inline char* EndOfString(char* str) { return str + strlen(str); }; inline char const* EndOfString(char const* str) { return str + strlen(str); };
