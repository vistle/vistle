#include "stdafx.h"
#include "MASTER.h"
 #define ___3860
#include "GLOBAL.h"
#include "CodeContract.h"
#include "TASSERT.h"
#include "Q_UNICODE.h"
#include "ARRLIST.h"
#include "STRLIST.h"
#include "CHARTYPE.h"
#include "STRUTIL.h"
#include "ALLOC.h"
#include "Q_MSG.h"
#include "ThirdPartyHeadersBegin.h"
#include "ThirdPartyHeadersEnd.h"
#include <algorithm>
#include <cctype> 
#include <limits.h>
#include "TranslatedString.h"
#include "stringformat.h"
using std::string; using tecplot::___4215; using tecplot::___1095; using tecplot::___4216;
 #ifdef MSWIN
 # pragma warning (disable : 4786) 
 #endif
 #define           FORMAT_BUFFER_SIZE 16384
static char      *FormatStringBuffer = NULL; static int        FormatStringBufferSize = FORMAT_BUFFER_SIZE; void ___1474() { if (FormatStringBuffer != NULL) free(FormatStringBuffer); FormatStringBuffer = NULL; } char *___4407(const char *Format, va_list     ___93) { char *___3357 = NULL; REQUIRE(VALID_REF(Format)); if (FormatStringBuffer == NULL) FormatStringBuffer = (char *)malloc(FormatStringBufferSize); if (FormatStringBuffer != NULL) {
 # if defined MSWIN
ASSERT_ONLY(int nChars =) _vsnprintf(FormatStringBuffer, FormatStringBufferSize, Format, ___93);
 # else
ASSERT_ONLY(int nChars =) vsnprintf(FormatStringBuffer, FormatStringBufferSize, Format, ___93);
 # endif
FormatStringBuffer[FormatStringBufferSize-1] = '\0'; ___476(nChars > 0); ___3357 = ___1133(___1095(FormatStringBuffer)); } ENSURE(VALID_REF(___3357) || ___3357 == NULL); return ___3357; } char *___1473(___4216 Format, ...) { REQUIRE(!Format.___2033()); va_list ___93;
 #if defined(_MSC_VER)
 #pragma warning (push, 0)
 #pragma warning (disable:4840)
 #endif
va_start(___93, Format);
 #if defined(_MSC_VER)
 #pragma warning (pop)
 #endif
char *___3357 = ___4407(Format.c_str(), ___93); va_end(___93); ENSURE(VALID_REF(___3357) || ___3357 == NULL); return ___3357; } int ___1473(string&           ___417, ___4216  Format ...) { REQUIRE(!Format.___2033()); va_list ___93;
 #if defined(_MSC_VER)
 #pragma warning (push, 0)
 #pragma warning (disable:4840)
 #endif
va_start(___93, Format);
 #if defined(_MSC_VER)
 #pragma warning (pop)
 #endif
char *FormattedString = ___4407(Format.c_str(), ___93); va_end(___93); int ___3357; if (FormattedString != NULL) { ___417.assign(FormattedString); ___3357 = (int)___417.size(); ___1528(FormattedString, "FormattedString"); } else ___3357 = -1; ENSURE(___3357 == -1 || ___3357 >= 0); return ___3357; } char *___1133(___4216 ___3811) { REQUIRE(VALID_TRANSLATED_STRING(___3811)); char *___3357 = ___23(strlen(___3811.c_str()) + 1, char, "duplicate string"); if (___3357 != NULL) strcpy(___3357, ___3811.c_str()); ENSURE(___3357 == NULL || (VALID_REF(___3357) && strcmp(___3357, ___3811.c_str()) == 0)); return ___3357; } void ___676(char       *___3944, const char *___3640, int         ___1924, int         ___682) { int       ___2222 = 0; REQUIRE(VALID_REF(___3944)); REQUIRE("Target string is sized to accommodate a string who's length " "is at least MIN(strlen(&Source[Index]), Count) characters."); REQUIRE(VALID_REF(___3640)); REQUIRE(0 <= ___1924 && ___1924 <= (int)strlen(___3640)); REQUIRE(___682 >= 0); ___2222 = MIN((int)strlen(&___3640[___1924]), ___682); memmove(___3944, &___3640[___1924], ___2222); ___3944[___2222] = '\0'; ENSURE(VALID_REF(___3944) && (int)strlen(___3944) == ___2222); } char *___3816(char *___3811) { char *___3357 = ___3811; char *Start = ___3811; REQUIRE(VALID_REF(___3811)); while (tecplot::isspace(*Start)) Start++; if (Start != ___3811) memmove(___3811, Start, strlen(Start) + 1); ENSURE(VALID_REF(___3357) && ___3357 == ___3811); return ___3357; } static char* StringFlushRight(char *___3811) { REQUIRE(VALID_REF(___3811)); char* ___3357 = ___3811; char* End = ___3811 + strlen(___3811) - 1; while (End > ___3811 && tecplot::isspace(*End)) End--; *(End + 1) = '\0'; ENSURE(VALID_REF(___3357) && ___3357 == ___3811); return ___3357; } char *___4223(char *___3811) { REQUIRE((___3811 == NULL) || VALID_REF(___3811)); if (___3811) return (___3816(StringFlushRight(___3811))); else return ___3811; } char *___3857(char  *___3811, int    ___2374) { REQUIRE(VALID_REF(___3811)); REQUIRE(___2374 >= 0); if ((int)strlen(___3811) > ___2374) ___3811[___2374] = '\0'; ENSURE(VALID_REF(___3811)); ENSURE((int)strlen(___3811) <= ___2374); return ___3811; } std::string& ___3856( std::string& str, size_t       maxLength) { if (!str.empty()) { ___3856(&str[0], static_cast<int>(maxLength)); str.resize(std::min(strlen(&str[0]), maxLength)); } return str; } char* ___3856( char* ___3811, int   ___2374) { REQUIRE(VALID_REF(___3811)); REQUIRE(___2374 >= 0); ___3816(___3811); ___3857(___3811,___2374); StringFlushRight(___3811); ENSURE(VALID_REF(___3811)); ENSURE((int)strlen(___3811) <= ___2374); ENSURE(IMPLICATION(strlen(___3811) != 0, (!tecplot::isspace(___3811[0]) && !tecplot::isspace(___3811[strlen(___3811)-1])))); return ___3811; }
 #ifndef MSWIN
___3837 ___2243(const char *___3811, uint32_t    ___4474) { REQUIRE(VALID_REF(___3811)); ___3837 ___3357 = ___3819(); if (___3357 != NULL) { ___372 ___2038 = ___4224; if (strlen(___3811) > ___4474) { char *StringCopy = ___1133(___1095(___3811)); ___2038 = (StringCopy != NULL); if (___2038) { char *___683 = StringCopy; char *SubString = StringCopy; uint32_t SubStringLen = 0; while (*___683 != '\0' && ___2038) { while (*___683 != '\0' && SubStringLen < ___4474) { if (*___683 == '\n') { *___683 = '\0'; ___683++; break; } ___683++; SubStringLen++; } if (*___683 != '\0' && SubStringLen == ___4474) { if (*___683 != ' ') { while (___683 != SubString && *___683 != ' ') ___683--; if (*___683 != ' ') { while (*___683 != '\0' && *___683 != ' ' && *___683 != '\n') ___683++; while (*___683 != '\0' && *___683 == ' ') ___683++; } } if (*___683 != '\0') { *___683 = '\0'; ___683++; } StringFlushRight(SubString); } ___2038 = ___3821(___3357, SubString); SubString = ___683; SubStringLen = 0; } ___1528(StringCopy, "StringCopy"); } } else ___2038 = ___3821(___3357, ___3811); if (!___2038) ___3826(&___3357); } ENSURE(___3357 == NULL || VALID_REF(___3357)); return ___3357; }
 #endif
int ustrncmp(const  char *___3428, const  char *___3429, size_t ___2221) { REQUIRE((___3428 == NULL) || VALID_REF(___3428)); REQUIRE((___3429 == NULL) || VALID_REF(___3429)); char *t1; char *t2; char ct1; char ct2; size_t ___1830 = 0; if ((___3428 == NULL) && (___3429 == NULL)) return 0; if (___3428 == NULL) return -1; else if (___3429 == NULL) return 1; t1 = (char*)___3428; t2 = (char*)___3429; while (*t1 && *t2 && (___1830 < ___2221)) { ct1 = ___442(*t1); ct2 = ___442(*t2); if (ct1 != ct2) return (ct1 - ct2); t1++; t2++; ___1830++; } if ((___1830 == ___2221) || ((*t1 == '\0') && (*t2 == '\0'))) return 0; else return ___442(*t1) - ___442(*t2); } int ustrcmp(const char *___3428, const char *___3429) { REQUIRE((___3428 == NULL) || VALID_REF(___3428)); REQUIRE((___3429 == NULL) || VALID_REF(___3429)); return (ustrncmp(___3428, ___3429, INT_MAX)); }
 #if !defined NO_ASSERTS
___372 ___1980(char       **___3431, const char  *___2698, ___372    ___2061, const char  *___1393, int          ___2260)
 #else
___372 ___1980(char       **___3431, const char  *___2698, ___372    ___2061)
 #endif
{ REQUIRE(VALID_REF(___3431)); REQUIRE(*___3431 == NULL || VALID_REF(*___3431)); REQUIRE(___2698 == NULL || VALID_REF(___2698)); REQUIRE(IMPLICATION(VALID_REF(*___3431), *___3431 != ___2698)); REQUIRE(VALID_BOOLEAN(___2061)); REQUIRE(VALID_NON_ZERO_LEN_STR(___1393)); REQUIRE(___2260 >= 1); if (*___3431) {
 #if !defined NO_ASSERTS && defined DEBUG_ALLOC
char S[80+1]; MakeDebugRecord(___1393, ___2260, "releasing", *___3431, S, 80); ___1528(*___3431, S);
 #else
___1528(*___3431, "");
 #endif
} if (___2698 == NULL) { *___3431 = NULL; return (___4224); } else {
 #if !defined NO_ASSERTS && defined DEBUG_ALLOC
char S[80+1]; MakeDebugRecord(___1393, ___2260, "duplicating", ___2698, S, 80); *___3431 = ___23(strlen(___2698) + 1, char, S);
 #else
*___3431 = ___23(strlen(___2698) + 1, char, "");
 #endif
if (*___3431) { strcpy(*___3431, ___2698); return (___4224); } else { if (___2061) ___1175(___4215("Out of memory")); return (___1303); } } } ___372 ___3939( char**      ___3431, char const* ___3854, ___372   ___956, ___372   convertNewlineToEscSeq) { size_t      CurLen; size_t      NewLen; int         NumNewlines = 0; char       *___2698; const char *___683 = ___3854; ___372   ___2038 = ___4224; REQUIRE(VALID_REF(___3431)); REQUIRE((___3854 == NULL) || VALID_REF(___3854)); REQUIRE(VALID_BOOLEAN(___956)); REQUIRE(VALID_BOOLEAN(convertNewlineToEscSeq)); if ((___3854  == NULL) || (*___3854 == '\0')) { if (___3854            && (*___3854 == '\0') && ___956) { char *TMP = (char *)___3854; ___1528(TMP, "empty string to add"); } } else { if (*___3431 == NULL) CurLen = 0; else CurLen = strlen(*___3431); while (*___683) if (*___683++ == '\n') NumNewlines++; NewLen = CurLen + strlen(___3854) + 1 + NumNewlines; ___2698 = ___23(NewLen, char, ___3854); if (___2698 == NULL) { if (___956) { char *TMP = (char *)___3854; ___1528(TMP, ___3854); } ___2038 = ___1303; } else { if (*___3431) { strcpy(___2698, *___3431); ___1528(*___3431, (CurLen > 0 ? *___3431 : "previous text")); } else *___2698 = '\0'; { char *NPtr = EndOfString(___2698); const char *APtr = ___3854; while (*APtr) { if ((*APtr == '\n') && convertNewlineToEscSeq) { *NPtr++ = '\\'; *NPtr++ = 'n'; } else *NPtr++ = *APtr; APtr++; } *NPtr = '\0'; } if (___956) { char *TMP = (char *)___3854; ___1528(TMP, ___3854); } *___3431 = ___2698; } } ENSURE(VALID_BOOLEAN(___2038)); return (___2038); } ___372 ___3938( char**      ___3431, char const* ___3854, ___372   convertNewlineToEscSeq) { size_t      CurLen; size_t      NewLen; int         NumNewlines = 0; char       *___2698; const char *___683 = ___3854; ___372   ___2038 = ___4224; REQUIRE(VALID_REF(___3431)); REQUIRE((___3854 == NULL) || VALID_REF(___3854)); REQUIRE(VALID_BOOLEAN(convertNewlineToEscSeq)); if ((___3854  != NULL) && (*___3854 != '\0')) { if (*___3431 == NULL) CurLen = 0; else CurLen = strlen(*___3431); while (*___683) if (*___683++ == '\n') NumNewlines++; NewLen = CurLen + strlen(___3854) + 1 + NumNewlines; ___2698 = ___23(NewLen, char, ___3854); if (___2698 == NULL) { ___2038 = ___1303; } else { if (*___3431) { strcpy(___2698, *___3431); ___1528(*___3431, (CurLen > 0 ? *___3431 : "previous text")); } else *___2698 = '\0'; { char *NPtr = EndOfString(___2698); const char *APtr = ___3854; while (*APtr) { if ((*APtr == '\n') && convertNewlineToEscSeq) { *NPtr++ = '\\'; *NPtr++ = 'n'; } else *NPtr++ = *APtr; APtr++; } *NPtr = '\0'; } *___3431 = ___2698; } } ENSURE(VALID_BOOLEAN(___2038)); return (___2038); } ___372 ___3937(char  **___3431, char    ___474) { REQUIRE(VALID_REF(___3431)); char S[2]; S[0] = ___474; S[1] = '\0'; return (___3939(___3431, S, ___1303, ___1303)); } ___372 ___3351(char **___3811)
{ size_t    ___1830; int       NewlineCount; size_t    ___2222; char     *Replacement; REQUIRE(VALID_REF(___3811)); REQUIRE(VALID_REF(*___3811)); NewlineCount = 0; ___2222 = strlen(*___3811); for (___1830 = 0; ___1830 < ___2222; ___1830++) if ((*___3811)[___1830] == '\n') NewlineCount++; if (NewlineCount != 0) { Replacement = ___23(___2222 + NewlineCount + 1, char, "replacement string"); if (Replacement != NULL) { size_t ___2104; for (___1830 = ___2104 = 0; ___1830 < ___2222 + 1; ___1830++, ___2104++) { if ((*___3811)[___1830] == '\n') { Replacement[___2104] = '\\'; ___2104++; Replacement[___2104] = 'n'; } else { Replacement[___2104] = (*___3811)[___1830]; } } ___476(___1830 == ___2222 + 1); ___476(___2104 == ___2222 + NewlineCount + 1); } ___1528(*___3811, "original string"); *___3811 = Replacement; } ENSURE(*___3811 == NULL || VALID_REF(*___3811)); return (*___3811 != NULL); }
