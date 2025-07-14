#include "stdafx.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "CodeContract.h"
#include "UnicodeStringUtils.h"
#include "ThirdPartyHeadersBegin.h"
#include <string>
#include "ThirdPartyHeadersEnd.h"
using std::string; using std::wstring; namespace tecplot { static inline string* createUtf8StringTranslation(string const& str) { string *___3356 = new string(str); ENSURE(VALID_REF(___3356)); return ___3356; }
 #if defined MSWIN
static inline wstring* createWideStringTranslation(string const& str) { wstring *___3356 = new wstring; *___3356 = utf8ToWideString(str.c_str()); ENSURE(VALID_REF(___3356)); return ___3356; }
 #endif
 #if defined MSWIN
static inline wstring* createWideString(___4216::___2503 ___2502, string const&          str) { REQUIRE(___2502 == ___4216::___1102 || ___2502 == ___4216::___1096); wstring* ___3356; if (___2502 == ___4216::___1102) ___3356 = createWideStringTranslation(str); else ___3356 = new wstring(utf8ToWideString(str.c_str())); return ___3356; }
 #endif
void ___4216::___1930(___4216::___2503 ___2502, char const*            str, char const*            ASSERT_ONLY(___4217)) { REQUIRE(___2502 == ___1102 || ___2502 == ___1096); REQUIRE(VALID_REF_OR_NULL(str)); REQUIRE(VALID_REF_OR_NULL(___4217)); ___2491   = ___2502; ___2485 = (str == NULL); if (!___2485) ___2621 = str; ___2664 = NULL;
 #if defined MSWIN
___2673 = NULL;
 #endif
} ___4216::___4216() { ___1930(___1096, NULL, NULL); ENSURE(this->___2065()); } ___4216 ___4216::___2763() { return ___1095(NULL); } ___4216::___4216(___4216::___2503 ___2502, char const*            str, char const*            ___4217) { REQUIRE(___2502 == ___1102 || ___2502 == ___1096); REQUIRE(VALID_REF_OR_NULL(str)); REQUIRE(VALID_REF_OR_NULL(___4217)); ___1930(___2502, str, ___4217); ENSURE(this->___2065()); } ___4216::~___4216() { delete ___2664;
 #if defined MSWIN
delete ___2673;
 #endif
}
 #if !defined NO_ASSERTS
bool ___4216::___2065() const { ___476(IMPLICATION(___2485, ___2621.length() == 0));
 #if 0
___476(IMPLICATION(___2485, ___2491 == ___1096));
 #endif
return true; }
 #endif
bool ___4216::___2033() const { INVARIANT(this->___2065()); return ___2485; } bool ___4216::___2034() const { INVARIANT(this->___2065()); return ___2485 || ___2621.length() == 0; } char const* ___4216::c_str() const { INVARIANT(this->___2065()); char const* ___3356 = NULL; if (!___2033()) { if (___2491 == ___1102) { if (___2664 == NULL) ___2664 = createUtf8StringTranslation(___2621); ___3356 = ___2664->c_str(); } else ___3356 = ___2621.c_str(); } ENSURE(___3356 == NULL || VALID_REF(___3356)); return ___3356; }
 #if defined MSWIN && !defined MAKEARCHIVE
wchar_t const* ___4216::___796() const { INVARIANT(this->___2065()); wchar_t const* ___3356 = NULL; if (!___2033()) { if (___2673 == NULL) ___2673 = createWideString(___2491, ___2621); ___3356 = ___2673->c_str(); } ENSURE(___3356 == NULL || VALID_REF(___3356)); return ___3356; }
 #endif
___4216::operator string() { INVARIANT(this->___2065()); REQUIRE(!___2033()); string* ___3356; if (___2491 == ___1102) { if (___2664 == NULL) ___2664 = createUtf8StringTranslation(___2621); ___3356 = ___2664; } else ___3356 = &___2621; return *___3356; }
 #if defined MSWIN && !defined MAKEARCHIVE
___4216::operator wstring() { INVARIANT(this->___2065()); REQUIRE(!___2033()); if (___2673 == NULL) ___2673 = createWideString(___2491, ___2621); return *___2673; }
 #endif
___4216& ___4216::operator=(___4216 const& ___2886) { REQUIRE(___2886.___2065()); if (this != &___2886) { ___2491       = ___2886.___2491; ___2485     = ___2886.___2485; ___2621     = ___2886.___2621; delete ___2664; ___2664 = (___2886.___2664 != NULL ? new string(*___2886.___2664) : NULL);
 #if defined MSWIN
delete ___2673; ___2673 = (___2886.___2673 != NULL ? new wstring(*___2886.___2673) : NULL);
 #endif
} ENSURE(this->___2065()); return *this; } ___4216::___4216(___4216 const& ___2886) { REQUIRE(___2886.___2065()); ___2491       = ___2886.___2491; ___2485     = ___2886.___2485; ___2621     = ___2886.___2621; ___2664 = (___2886.___2664 != NULL ? new string(*___2886.___2664) : NULL);
 #if defined MSWIN
___2673 = (___2886.___2673 != NULL ? new wstring(*___2886.___2673) : NULL);
 #endif
ENSURE(this->___2065()); } ___4216 ___4216::___4215(char const* str, char const* ___4217) { REQUIRE(VALID_REF_OR_NULL(str)); REQUIRE(VALID_REF_OR_NULL(___4217)); return ___4216(___1102, str, ___4217); } ___4216 ___4216::___1095(char const* str) { REQUIRE(VALID_REF_OR_NULL(str)); return ___4216(___1096, str, NULL); } }
